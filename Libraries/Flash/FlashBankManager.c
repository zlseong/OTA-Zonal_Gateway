/**********************************************************************************************************************
 * \file FlashBankManager.c
 * \brief Flash Bank Manager Implementation for TC375 Dual-Bank OTA
 * \copyright Copyright (C) Infineon Technologies AG 2019
 * 
 * Implements dual-bank firmware management for safe OTA updates
 *********************************************************************************************************************/

#include "FlashBankManager.h"
#include "Libraries/iLLD/TC3xx/Tricore/Flash/Std/IfxFlash.h"
#include "Libraries/iLLD/TC3xx/Tricore/Cpu/Std/IfxCpu.h"
#include "Libraries/UART/UART_Logging.h"
#include <string.h>
#include <stdio.h>

/*******************************************************************************
 * Private Variables
 ******************************************************************************/

/* Current bank status */
static FlashBankStatus_t g_bankStatus;
static FlashBank_t g_activeBank = FLASH_BANK_A;
static FlashBank_t g_inactiveBank = FLASH_BANK_B;
static boolean g_isInitialized = FALSE;

/* Synchronization progress tracking */
static volatile boolean g_syncInProgress = FALSE;
static volatile uint32 g_syncProgress = 0;  /* 0~100% */

/*******************************************************************************
 * Private Function Prototypes
 ******************************************************************************/

static uint32 FlashBank_CalculateCRC32(uint32 address, uint32 size);
boolean FlashBank_ReadDFlashStatus(FlashBankStatus_t *status);
boolean FlashBank_WriteDFlashStatus(FlashBankStatus_t status);
static void FlashBank_ServiceWatchdog(void);

/*******************************************************************************
 * Public Functions
 ******************************************************************************/

void FlashBank_Init(void)
{
    if (g_isInitialized)
    {
        return;
    }
    
    /* Read bank status from DFlash */
    if (!FlashBank_ReadDFlashStatus(&g_bankStatus))
    {
        /* First boot or DFlash corrupted - initialize */
        g_bankStatus.value = 0;
        g_bankStatus.bits.bootTarget = 0;          /* Boot from Bank A (default) */
        g_bankStatus.bits.statusA = BANK_STATUS_OK;      /* Bank A OK */
        g_bankStatus.bits.statusB = BANK_STATUS_OK;      /* Bank B OK (assume debugged) */
        g_bankStatus.bits.banksIdentical = 0;      /* Unknown, will check */
        g_bankStatus.bits.criticalError = 0;
        
        FlashBank_WriteDFlashStatus(g_bankStatus);
        sendUARTMessage("[FlashBankMgr] First boot: DFlash initialized\r\n", 47);
    }
    
    /* Determine active bank from boot target */
    g_activeBank = (g_bankStatus.bits.bootTarget == 0) ? FLASH_BANK_A : FLASH_BANK_B;
    g_inactiveBank = (g_activeBank == FLASH_BANK_A) ? FLASH_BANK_B : FLASH_BANK_A;
    
    g_isInitialized = TRUE;
    
    char log_msg[128];
    sprintf(log_msg, "[FlashBankMgr] Init: Active=%s, Identical=%d\r\n",
            (g_activeBank == FLASH_BANK_A) ? "Bank_A" : "Bank_B",
            g_bankStatus.bits.banksIdentical);
    sendUARTMessage(log_msg, strlen(log_msg));
    
    /* Quick synchronization check using metadata (fast!) */
    if (g_bankStatus.bits.banksIdentical == 0)
    {
        sendUARTMessage("[FlashBankMgr] Performing quick sync check (metadata only)...\r\n", 63);
        
        boolean synced = FlashBank_QuickSyncCheck();
        
        if (synced)
        {
            sendUARTMessage("[FlashBankMgr] Banks already synchronized!\r\n", 44);
            g_bankStatus.bits.banksIdentical = 1;
            FlashBank_WriteDFlashStatus(g_bankStatus);
        }
        else
        {
            sendUARTMessage("[FlashBankMgr] Banks differ (sync needed in background)\r\n", 59);
        }
    }
    else
    {
        sendUARTMessage("[FlashBankMgr] Banks already synchronized (flag set)\r\n", 55);
    }
}

ResetType_t FlashBank_GetResetType(void)
{
    /* Read SCU_STMEM3 register */
    uint32 resetType = *(volatile uint32 *)SCU_STMEM3_ADDRESS;
    
    /* Decode reset type */
    if (resetType == 0xA030F81F)
    {
        return RESET_TYPE_COLD_POWER_ON;
    }
    else if (resetType == 0xA020F82F)
    {
        return RESET_TYPE_WARM_POWER_ON;
    }
    else if (resetType == 0x2020B84F)
    {
        return RESET_TYPE_SYSTEM_RESET;
    }
    else if (resetType == 0x2020D88F)
    {
        return RESET_TYPE_APPLICATION_RESET;
    }
    
    return RESET_TYPE_UNKNOWN;
}

FlashBank_t FlashBank_GetActiveBoot(void)
{
    return g_activeBank;
}

FlashBank_t FlashBank_GetGoldenBank(void)
{
    return g_inactiveBank;
}

FlashBankStatus_t FlashBank_GetStatusFlags(void)
{
    return g_bankStatus;
}

boolean FlashBank_SetStatusFlags(FlashBankStatus_t flags)
{
    g_bankStatus = flags;
    return FlashBank_WriteDFlashStatus(flags);
}

/*******************************************************************************
 * SW Synchronization: Active Bank → Inactive Bank
 ******************************************************************************/

boolean FlashBank_SWSynchronization(void)
{
    if (!g_isInitialized)
    {
        sendUARTMessage("[FlashBankMgr] ERROR: Not initialized!\r\n", 42);
        return FALSE;
    }
    
    if (g_syncInProgress)
    {
        sendUARTMessage("[FlashBankMgr] WARNING: Sync already in progress\r\n", 51);
        return FALSE;
    }
    
    /* Check if already synchronized */
    if (g_bankStatus.bits.banksIdentical == 1)
    {
        sendUARTMessage("[FlashBankMgr] Banks already synchronized\r\n", 43);
        return TRUE;
    }
    
    /* Set syncInProgress flag in DFlash */
    g_bankStatus.bits.syncInProgress = 1;
    FlashBank_WriteDFlashStatus(g_bankStatus);
    
    g_syncInProgress = TRUE;
    g_syncProgress = 0;
    
    char log_msg[128];
    sprintf(log_msg, "[FlashBankMgr] SW Sync START: %s → %s\r\n",
            (g_activeBank == FLASH_BANK_A) ? "Bank_A" : "Bank_B",
            (g_inactiveBank == FLASH_BANK_A) ? "Bank_A" : "Bank_B");
    sendUARTMessage(log_msg, strlen(log_msg));
    
    /* Get source and destination addresses */
    uint32 srcAddr = (g_activeBank == FLASH_BANK_A) ? PFLASH_BANK_A_START : PFLASH_BANK_B_START;
    uint32 dstAddr = (g_inactiveBank == FLASH_BANK_A) ? PFLASH_BANK_A_START : PFLASH_BANK_B_START;
    uint32 totalSize = PFLASH_BANK_A_SIZE;  /* 3MB */
    
    /* TC375 Flash Sector Size: 16KB */
    #define FLASH_SECTOR_SIZE       (16 * 1024)
    #define FLASH_PAGE_SIZE         256
    #define TOTAL_SECTORS           (totalSize / FLASH_SECTOR_SIZE)
    
    uint32 srcCrc = 0;
    uint32 dstCrc = 0;
    
    /* ===== Step 1: Calculate source CRC ===== */
    sendUARTMessage("[FlashBankMgr] Step 1/4: Calculating source CRC...\r\n", 54);
    srcCrc = FlashBank_CalculateCRC32(srcAddr, totalSize);
    sprintf(log_msg, "[FlashBankMgr] Source CRC32: 0x%08X\r\n", (unsigned int)srcCrc);
    sendUARTMessage(log_msg, strlen(log_msg));
    g_syncProgress = 10;
    
    /* ===== Step 2: Erase destination bank ===== */
    sendUARTMessage("[FlashBankMgr] Step 2/4: Erasing destination bank...\r\n", 56);
    
    for (uint32 sector = 0; sector < TOTAL_SECTORS; sector++)
    {
        uint32 sectorAddr = dstAddr + (sector * FLASH_SECTOR_SIZE);
        
        /* Erase sector */
        IfxFlash_eraseSector(sectorAddr);
        
        /* Wait for erase completion */
        IfxFlash_waitUnbusy(0, IfxFlash_FlashType_P0);
        
        /* Verify erase */
        IfxFlash_eraseVerifySector(sectorAddr);
        IfxFlash_waitUnbusy(0, IfxFlash_FlashType_P0);
        
        /* Update progress: 10% ~ 30% */
        g_syncProgress = 10 + ((sector + 1) * 20 / TOTAL_SECTORS);
        
        /* Refresh watchdog every 16 sectors (~256KB) */
        if ((sector % 16) == 0)
        {
            FlashBank_ServiceWatchdog();
        }
    }
    
    sendUARTMessage("[FlashBankMgr] Erase complete\r\n", 32);
    g_syncProgress = 30;
    
    /* ===== Step 3: Copy data sector by sector ===== */
    sendUARTMessage("[FlashBankMgr] Step 3/4: Copying data...\r\n", 43);
    
    for (uint32 sector = 0; sector < TOTAL_SECTORS; sector++)
    {
        uint32 srcSectorAddr = srcAddr + (sector * FLASH_SECTOR_SIZE);
        uint32 dstSectorAddr = dstAddr + (sector * FLASH_SECTOR_SIZE);
        
        /* Copy sector page by page (256 bytes per page) */
        for (uint32 pageOffset = 0; pageOffset < FLASH_SECTOR_SIZE; pageOffset += FLASH_PAGE_SIZE)
        {
            uint32 srcPageAddr = srcSectorAddr + pageOffset;
            uint32 dstPageAddr = dstSectorAddr + pageOffset;
            
            /* Enter page mode */
            IfxFlash_enterPageMode(dstPageAddr);
            
            /* Load page data (32-bit words, 64 words = 256 bytes) */
            volatile uint32 *srcPtr = (volatile uint32 *)srcPageAddr;
            volatile uint32 *dstPtr = (volatile uint32 *)dstPageAddr;
            
            for (uint32 word = 0; word < (FLASH_PAGE_SIZE / 4); word++)
            {
                *dstPtr++ = *srcPtr++;
            }
            
            /* Write page */
            IfxFlash_writePage(dstPageAddr);
            
            /* Wait for write completion */
            IfxFlash_waitUnbusy(0, IfxFlash_FlashType_P0);
        }
        
        /* Update progress: 30% ~ 80% */
        g_syncProgress = 30 + ((sector + 1) * 50 / TOTAL_SECTORS);
        
        /* Refresh watchdog every sector */
        if ((sector % 4) == 0)
        {
            FlashBank_ServiceWatchdog();
            
            /* Log progress every 64 sectors (~1MB) */
            if ((sector % 64) == 0)
            {
                sprintf(log_msg, "[FlashBankMgr] Progress: %u%%\r\n", (unsigned int)g_syncProgress);
                sendUARTMessage(log_msg, strlen(log_msg));
            }
        }
    }
    
    sendUARTMessage("[FlashBankMgr] Copy complete\r\n", 31);
    g_syncProgress = 80;
    
    /* ===== Step 4: Verify destination CRC ===== */
    sendUARTMessage("[FlashBankMgr] Step 4/4: Verifying destination CRC...\r\n", 57);
    dstCrc = FlashBank_CalculateCRC32(dstAddr, totalSize);
    sprintf(log_msg, "[FlashBankMgr] Destination CRC32: 0x%08X\r\n", (unsigned int)dstCrc);
    sendUARTMessage(log_msg, strlen(log_msg));
    g_syncProgress = 95;
    
    /* ===== CRC Verification ===== */
    if (srcCrc != dstCrc)
    {
        sendUARTMessage("[FlashBankMgr] ERROR: CRC mismatch!\r\n", 38);
        g_bankStatus.bits.syncInProgress = 0;  /* Clear sync flag */
        FlashBank_WriteDFlashStatus(g_bankStatus);
        g_syncInProgress = FALSE;
        g_syncProgress = 0;
        return FALSE;
    }
    
    /* ===== Update status flags ===== */
    g_bankStatus.bits.banksIdentical = 1;
    g_bankStatus.bits.statusA = BANK_STATUS_OK;
    g_bankStatus.bits.statusB = BANK_STATUS_OK;
    g_bankStatus.bits.syncInProgress = 0;  /* Clear sync flag */
    FlashBank_WriteDFlashStatus(g_bankStatus);
    
    g_syncProgress = 100;
    sendUARTMessage("[FlashBankMgr] SW Sync SUCCESS: Banks are identical!\r\n", 56);
    
    g_syncInProgress = FALSE;
    return TRUE;
}

/*******************************************************************************
 * Private Helper Functions
 ******************************************************************************/

/**
 * \brief Calculate CRC32 for a memory region
 * 
 * Uses software CRC32 (IEEE 802.3 polynomial: 0x04C11DB7)
 * For production, consider using hardware FCE module for speed
 */
static uint32 FlashBank_CalculateCRC32(uint32 address, uint32 size)
{
    uint32 crc = 0xFFFFFFFF;
    const uint32 polynomial = 0xEDB88320;  /* Reversed polynomial */
    
    volatile uint8 *data = (volatile uint8 *)address;
    
    for (uint32 i = 0; i < size; i++)
    {
        crc ^= data[i];
        
        for (uint32 bit = 0; bit < 8; bit++)
        {
            if (crc & 1)
            {
                crc = (crc >> 1) ^ polynomial;
            }
            else
            {
                crc = crc >> 1;
            }
        }
        
        /* Refresh watchdog every 64KB */
        if ((i % 65536) == 0 && i != 0)
        {
            FlashBank_ServiceWatchdog();
        }
    }
    
    return ~crc;
}

/**
 * \brief Read bank status from DFlash
 */
boolean FlashBank_ReadDFlashStatus(FlashBankStatus_t *status)
{
    if (status == NULL)
    {
        return FALSE;
    }
    
    /* Read from DFlash */
    volatile uint8 *dflashAddr = (volatile uint8 *)(DFLASH_BASE_ADDRESS + DFLASH_BANK_STATUS_OFFSET);
    status->value = *dflashAddr;
    
    return TRUE;
}

/**
 * \brief Write bank status to DFlash
 */
boolean FlashBank_WriteDFlashStatus(FlashBankStatus_t status)
{
    uint32 dflashAddr = DFLASH_BASE_ADDRESS + DFLASH_BANK_STATUS_OFFSET;
    
    /* Enter page mode for DFlash */
    IfxFlash_enterPageMode(dflashAddr);
    
    /* Load data */
    volatile uint8 *dst = (volatile uint8 *)dflashAddr;
    *dst = status.value;
    
    /* Write page */
    IfxFlash_writePage(dflashAddr);
    
    /* Wait for completion */
    IfxFlash_waitUnbusy(0, IfxFlash_FlashType_D0);
    
    return TRUE;
}

/**
 * \brief Service watchdog to prevent timeout during long operations
 */
static void FlashBank_ServiceWatchdog(void)
{
    /* TODO: Implement actual watchdog service */
    /* For now, just a placeholder */
    __asm("nop");
}

/*******************************************************************************
 * Bank Metadata Management
 ******************************************************************************/

/**
 * \brief Read metadata from a specific bank
 */
boolean FlashBank_ReadMetadata(FlashBank_t bank, BankMetadata_t *metadata)
{
    if (metadata == NULL)
    {
        return FALSE;
    }
    
    uint32 bankAddr = (bank == FLASH_BANK_A) ? PFLASH_BANK_A_START : PFLASH_BANK_B_START;
    
    /* Read metadata from bank (first 256 bytes) */
    volatile BankMetadata_t *flashMetadata = (volatile BankMetadata_t *)bankAddr;
    
    /* Copy to RAM */
    memcpy(metadata, (const void *)flashMetadata, sizeof(BankMetadata_t));
    
    /* Validate magic number */
    if (metadata->magic_number != BANK_METADATA_MAGIC)
    {
        return FALSE;  /* Invalid or erased bank */
    }
    
    return TRUE;
}

/**
 * \brief Write metadata to a specific bank
 */
boolean FlashBank_WriteMetadata(FlashBank_t bank, const BankMetadata_t *metadata)
{
    if (metadata == NULL)
    {
        return FALSE;
    }
    
    uint32 bankAddr = (bank == FLASH_BANK_A) ? PFLASH_BANK_A_START : PFLASH_BANK_B_START;
    
    /* Write metadata (first 256 bytes) */
    /* Note: In real implementation, Flash programming is needed */
    /* This is a placeholder - actual Flash write requires IfxFlash API */
    
    sendUARTMessage("[FlashBankMgr] WriteMetadata: Flash programming not implemented yet\r\n", 71);
    
    return FALSE;  /* TODO: Implement Flash write */
}

/**
 * \brief Quick check if banks are synchronized (using metadata)
 */
boolean FlashBank_QuickSyncCheck(void)
{
    BankMetadata_t metadataA;
    BankMetadata_t metadataB;
    
    /* Read metadata from both banks */
    boolean validA = FlashBank_ReadMetadata(FLASH_BANK_A, &metadataA);
    boolean validB = FlashBank_ReadMetadata(FLASH_BANK_B, &metadataB);
    
    /* If either bank is invalid, not synchronized */
    if (!validA || !validB)
    {
        char log[64];
        sprintf(log, "[FlashBankMgr] QuickSync: A=%d, B=%d (invalid)\r\n", validA, validB);
        sendUARTMessage(log, strlen(log));
        return FALSE;
    }
    
    /* Compare key metadata fields */
    boolean same_version = (metadataA.firmware_version == metadataB.firmware_version);
    boolean same_crc = (metadataA.crc32 == metadataB.crc32);
    boolean same_timestamp = (metadataA.build_timestamp == metadataB.build_timestamp);
    boolean same_size = (metadataA.total_size == metadataB.total_size);
    
    char log[128];
    sprintf(log, "[FlashBankMgr] QuickSync: Ver=%d, CRC=%d, Time=%d, Size=%d\r\n",
            same_version, same_crc, same_timestamp, same_size);
    sendUARTMessage(log, strlen(log));
    
    /* All must match for synchronization */
    return (same_version && same_crc && same_timestamp && same_size);
}

/*******************************************************************************
 * UCB BMHD Management - Boot Target Switching
 ******************************************************************************/

/**
 * \brief Update Boot Mode Header (BMHD) to switch boot bank
 * 
 * ⚠️ CRITICAL FUNCTION - Modifies hardware boot configuration
 * 
 * This function updates UCB_BMHD0 to change the boot start address.
 * TC375 Boot ROM reads BMHD at power-on to determine where to start execution.
 * 
 * UCB Limitations:
 * - Limited write cycles (~100 times typically)
 * - Once corrupted, device may be bricked
 * - Requires Endinit password protection
 * 
 * For production, consider:
 * - Using a fixed bootloader that reads DFlash flags
 * - Implementing write counters
 * - Adding extensive validation
 */
boolean FlashBank_UpdateBootTarget(FlashBank_t targetBank)
{
    sendUARTMessage("[FlashBankMgr] WARNING: UCB_BMHD update requested\r\n", 53);
    sendUARTMessage("[FlashBankMgr] This operation has limited cycles!\r\n", 52);
    
    /* Determine target start address */
    uint32 targetAddress = (targetBank == FLASH_BANK_A) ? PFLASH_BANK_A_START : PFLASH_BANK_B_START;
    
    char log_msg[128];
    sprintf(log_msg, "[FlashBankMgr] Target Boot Address: 0x%08X\r\n", (unsigned int)targetAddress);
    sendUARTMessage(log_msg, strlen(log_msg));
    
    /***************************************************************************
     * UCB_BMHD0 Structure (Simplified)
     * 
     * Offset 0x00: STAD (Start Address) - 32-bit
     * Offset 0x04: Reserved
     * Offset 0x08: CRCBMHD (CRC of BMHD) - 32-bit
     * ...
     * 
     * Total: 512 bytes (0xAF400000 ~ 0xAF4001FF)
     ***************************************************************************/
    
    /* UCB BMHD is complex and dangerous to modify */
    /* For now, we'll use DFlash flags + Software-based boot selection */
    
    sendUARTMessage("[FlashBankMgr] NOTICE: Using DFlash flag method instead\r\n", 58);
    sendUARTMessage("[FlashBankMgr] Actual UCB_BMHD modification requires:\r\n", 57);
    sendUARTMessage("[FlashBankMgr]   1. Bootloader implementation\r\n", 48);
    sendUARTMessage("[FlashBankMgr]   2. BMHD CRC calculation\r\n", 43);
    sendUARTMessage("[FlashBankMgr]   3. Password protection handling\r\n", 51);
    sendUARTMessage("[FlashBankMgr]   4. Write cycle management\r\n", 46);
    
    /* Update DFlash flags instead (safe method) */
    FlashBankStatus_t status = FlashBank_GetStatusFlags();
    
    if (targetBank == FLASH_BANK_A)
    {
        status.bits.bootTarget = 0;
    }
    else
    {
        status.bits.bootTarget = 1;
    }
    
    FlashBank_WriteDFlashStatus(status);
    
    sendUARTMessage("[FlashBankMgr] DFlash boot flag updated\r\n", 42);
    sendUARTMessage("[FlashBankMgr] NOTE: Requires bootloader to read DFlash flags\r\n", 65);
    
    return TRUE;
}

