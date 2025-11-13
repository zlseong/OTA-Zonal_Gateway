/**********************************************************************************************************************
 * \file FlashBankManager.h
 * \copyright Copyright (C) Infineon Technologies AG 2019
 * 
 * Flash Bank Manager for TC375 Dual-Bank OTA Update
 * 
 * TC375 PFlash Configuration:
 *   - PFlash0 (Bank A): 0xA000_0000 - 0xA02F_FFFF (3MB)
 *   - PFlash1 (Bank B): 0xA030_0000 - 0xA05F_FFFF (3MB)
 *   - Total: 6MB (2 banks × 3MB each)
 * 
 * Usage:
 *   1. Determine active boot bank
 *   2. Download firmware to inactive bank
 *   3. Verify downloaded firmware
 *   4. Switch boot bank and reset
 *********************************************************************************************************************/

#ifndef FLASH_BANK_MANAGER_H_
#define FLASH_BANK_MANAGER_H_

#include "Ifx_Types.h"

/*******************************************************************************
 * Flash Bank Memory Map (TC375)
 ******************************************************************************/

/* PFlash Bank A (3MB) */
#define PFLASH_BANK_A_START         0xA0000000
#define PFLASH_BANK_A_SIZE          0x00300000  /* 3MB */
#define PFLASH_BANK_A_END           (PFLASH_BANK_A_START + PFLASH_BANK_A_SIZE - 1)

/* PFlash Bank B (3MB) */
#define PFLASH_BANK_B_START         0xA0300000
#define PFLASH_BANK_B_SIZE          0x00300000  /* 3MB */
#define PFLASH_BANK_B_END           (PFLASH_BANK_B_START + PFLASH_BANK_B_SIZE - 1)

/* Cached access addresses (for execution) */
#define PFLASH_CACHED_BASE          0x80000000
#define PFLASH_BANK_A_CACHED        (PFLASH_CACHED_BASE + 0x00000000)
#define PFLASH_BANK_B_CACHED        (PFLASH_CACHED_BASE + 0x00300000)

/* User Configuration Blocks (UCB) */
#define UCB_BASE_ADDRESS            0xAF400000
#define UCB_BMHD0_ADDRESS           0xAF400000  /* Boot Mode Header 0 */
#define UCB_BMHD1_ADDRESS           0xAF400200  /* Boot Mode Header 1 */
#define UCB_SIZE                    0x00006000  /* 24KB */

/* Data Flash for Bank Status Storage */
#define DFLASH_BASE_ADDRESS         0xAFC00000  /* Corrected: DFlash actual address */
#define DFLASH_BANK_STATUS_OFFSET   0x00000000  /* Bank status at start of DFlash */
#define DFLASH_SIZE                 0x00020000  /* 128KB (actual TC37x DFlash size) */

/* OLDA (Online Data Acquisition) for OTA Diagnostics (Optional) */
#define OLDA_BASE_ADDRESS_CACHED    0x8FE00000  /* Cached access */
#define OLDA_BASE_ADDRESS_PHYSICAL  0xAFE00000  /* Non-cached access */
#define OLDA_SIZE                   0x00080000  /* 512KB */
#define OLDA_OTA_LOG_OFFSET         0x00000000  /* OTA diagnostic log storage */

/* SCU (System Control Unit) - Reset Type Detection */
#define SCU_BASE_ADDRESS            0xF0036000
#define SCU_STMEM3_OFFSET           0x01C0  /* Application Reset detection */
#define SCU_STMEM4_OFFSET           0x01C4  /* Cold PowerOn Reset */
#define SCU_STMEM5_OFFSET           0x01C8  /* PowerOn Reset */
#define SCU_STMEM6_OFFSET           0x01CC  /* System Reset */

/* SCU_STMEMx absolute addresses */
#define SCU_STMEM3_ADDRESS          (SCU_BASE_ADDRESS + SCU_STMEM3_OFFSET)  /* 0xF00361C0 */
#define SCU_STMEM4_ADDRESS          (SCU_BASE_ADDRESS + SCU_STMEM4_OFFSET)  /* 0xF00361C4 */
#define SCU_STMEM5_ADDRESS          (SCU_BASE_ADDRESS + SCU_STMEM5_OFFSET)  /* 0xF00361C8 */
#define SCU_STMEM6_ADDRESS          (SCU_BASE_ADDRESS + SCU_STMEM6_OFFSET)  /* 0xF00361CC */

/*******************************************************************************
 * Type Definitions
 ******************************************************************************/

/**
 * \brief Flash Bank Status Flags (7-bit system)
 * 
 * Stored in Data Flash for fast access and frequent updates.
 * This structure survives power cycles and resets.
 * 
 * Storage Location: DFLASH_BASE_ADDRESS + DFLASH_BANK_STATUS_OFFSET
 */
typedef union
{
    uint8 value;  /* Combined 7-bit value */
    
    struct
    {
        uint8 currentBank     : 1;  /* Bit 0: Current active bank (0=Bank A, 1=Bank B) */
        uint8 bootFlagBankA   : 1;  /* Bit 1: Boot flag for Bank A (1=next boot target) */
        uint8 bootFlagBankB   : 1;  /* Bit 2: Boot flag for Bank B (1=next boot target) */
        uint8 errorBankA      : 1;  /* Bit 3: Error marker Bank A (1=error, 0=normal) */
        uint8 errorBankB      : 1;  /* Bit 4: Error marker Bank B (1=error, 0=normal) */
        uint8 banksIdentical  : 1;  /* Bit 5: Software A == B (1=identical, 0=different) */
        uint8 criticalError   : 1;  /* Bit 6: Both banks error (1=warning light ON) */
        uint8 reserved        : 1;  /* Bit 7: Reserved for future use */
    } bits;
} FlashBankStatus_t;

/**
 * \brief Reset Type enumeration (from SCU_STMEM3 register)
 * 
 * Used to determine the cause of the last reset, which helps decide
 * whether a rollback is needed during boot initialization.
 */
typedef enum
{
    RESET_TYPE_UNKNOWN = 0,
    RESET_TYPE_COLD_POWER_ON,     /* SCU_STMEM3 = 0xA030F81F */
    RESET_TYPE_WARM_POWER_ON,     /* SCU_STMEM3 = 0xA020F82F */
    RESET_TYPE_SYSTEM_RESET,      /* SCU_STMEM3 = 0x2020B84F */
    RESET_TYPE_APPLICATION_RESET  /* SCU_STMEM3 = 0x2020D88F (rollback trigger) */
} ResetType_t;

/**
 * \brief Flash Bank enumeration
 */
typedef enum
{
    FLASH_BANK_A = 0,  /* PFlash0 */
    FLASH_BANK_B = 1   /* PFlash1 */
} FlashBank_t;

/**
 * \brief Bank Status enumeration
 */
typedef enum
{
    BANK_STATUS_UNKNOWN = 0,      /* Status not determined yet */
    BANK_STATUS_EMPTY,            /* Bank is erased */
    BANK_STATUS_ACTIVE,           /* Bank is currently executing (not yet validated) */
    BANK_STATUS_GOLDEN,           /* Bank contains validated "Golden Image" (safe fallback) */
    BANK_STATUS_DOWNLOADING,      /* Bank is being programmed */
    BANK_STATUS_READY,            /* Bank is programmed and verified, ready to boot */
    BANK_STATUS_VALIDATED,        /* Bank has been booted and validated as stable */
    BANK_STATUS_ERROR             /* Bank has programming, boot, or runtime error */
} BankStatus_t;

/**
 * \brief Flash Bank Information structure
 */
typedef struct
{
    FlashBank_t bank;             /* Bank identifier */
    uint32 startAddress;          /* Physical start address */
    uint32 size;                  /* Bank size in bytes */
    BankStatus_t status;          /* Current bank status */
    uint32 firmwareVersion;       /* Firmware version stored in bank */
    uint32 firmwareCrc32;         /* CRC32 of firmware in bank */
} FlashBankInfo_t;

/*******************************************************************************
 * Function Prototypes
 ******************************************************************************/

/**
 * \brief Initialize Flash Bank Manager
 * 
 * This function must be called once during system initialization.
 * It reads the Boot Mode Header (BMHD) to determine which bank is currently active.
 * 
 * Design Rule (Normal Operation):
 *   - Bank A: Primary ACTIVE bank (default boot target)
 *   - Bank B: GOLDEN backup (safe fallback)
 * 
 * During Rollback:
 *   - Boot flag switches to Bank B temporarily
 *   - After validation, Bank A is restored from Bank B
 *   - Boot flag returns to Bank A
 * 
 * Boot Initialization Sequence:
 *   1. Check reset type (SCU_STMEM3)
 *   2. Read DFlash status flags
 *   3. Determine active boot bank from BMHD
 *   4. If rollback detected, start restoration process
 */
void FlashBank_Init(void);

/**
 * \brief Get last reset type from SCU registers
 * 
 * Reads SCU_STMEM3 register to determine the cause of the last reset.
 * This is useful for detecting rollback scenarios.
 * 
 * \return Reset type enumeration
 */
ResetType_t FlashBank_GetResetType(void);

/**
 * \brief Get currently active boot bank (from BMHD)
 * 
 * \return FLASH_BANK_A or FLASH_BANK_B based on Boot Mode Header
 */
FlashBank_t FlashBank_GetActiveBoot(void);

/**
 * \brief Get golden/backup bank (opposite of active)
 * 
 * \return FLASH_BANK_B if active is A, or FLASH_BANK_A if active is B
 */
FlashBank_t FlashBank_GetGoldenBank(void);

/**
 * \brief Get information about a specific flash bank
 * 
 * \param bank Target bank
 * \param info Pointer to structure to receive bank information
 * \return TRUE if successful, FALSE otherwise
 */
boolean FlashBank_GetInfo(FlashBank_t bank, FlashBankInfo_t* info);

/**
 * \brief Get status of a specific flash bank
 * 
 * \param bank Target bank
 * \return Bank status
 */
BankStatus_t FlashBank_GetStatus(FlashBank_t bank);

/**
 * \brief Set status of a specific flash bank
 * 
 * \param bank Target bank
 * \param status New status
 */
void FlashBank_SetStatus(FlashBank_t bank, BankStatus_t status);

/**
 * \brief Get current bank status flags (7-bit system)
 * 
 * Reads the status flags from Data Flash.
 * 
 * \return Current bank status flags
 */
FlashBankStatus_t FlashBank_GetStatusFlags(void);

/**
 * \brief Set bank status flags (7-bit system)
 * 
 * Writes the status flags to Data Flash.
 * Updates are immediate and persist across resets.
 * 
 * \param flags New status flags to write
 * \return TRUE if write successful, FALSE otherwise
 */
boolean FlashBank_SetStatusFlags(FlashBankStatus_t flags);

/**
 * \brief Update Boot Mode Header to switch boot target
 * 
 * Modifies UCB BMHD to change the next boot address.
 * 
 * \param targetBank Bank to boot from after reset
 * \return TRUE if BMHD update successful, FALSE otherwise
 */
boolean FlashBank_UpdateBootTarget(FlashBank_t targetBank);

/**
 * \brief Validate current active bank after successful boot and runtime check
 * 
 * This function should be called after the system has been running stably
 * for a certain period (e.g., 30 seconds ~ 1 minute) to confirm the firmware is working.
 * 
 * Process:
 *   1. Mark active bank as VALIDATED
 *   2. If inactive bank is GOLDEN, copy validated firmware to make new golden image
 *   3. Update boot configuration
 * 
 * Typical usage:
 *   - Call from main loop after stability check timer expires
 *   - Service center: After functional tests pass
 *   - OTA: After vehicle operates normally for defined duration
 * 
 * \return TRUE if validation successful, FALSE otherwise
 */
boolean FlashBank_ValidateActiveFirmware(void);

/**
 * \brief Check if current boot is the first boot of new firmware
 * 
 * Used to determine if we need to start the validation timer.
 * 
 * \return TRUE if this is first boot of unvalidated firmware, FALSE otherwise
 */
boolean FlashBank_IsFirstBoot(void);

/**
 * \brief Copy firmware from active bank to inactive bank (create Golden Image)
 * 
 * This function copies validated firmware to the inactive bank as a backup.
 * Only executed after FlashBank_ValidateActiveFirmware() succeeds.
 * 
 * \return TRUE if copy successful, FALSE otherwise
 */
boolean FlashBank_CreateGoldenImage(void);

/**
 * \brief Trigger rollback to Golden Image (switch boot flag)
 * 
 * This function is called when:
 *   - Boot validation fails (watchdog, startup errors)
 *   - Runtime critical error detected
 *   - Manual rollback requested
 * 
 * Process (Immediate - Fast Rollback):
 *   1. Mark current active bank as ERROR
 *   2. Switch Boot Flag to Golden bank (modify BMHD)
 *   3. Perform system reset
 * 
 * After reset:
 *   - System boots from Golden bank (working firmware)
 *   - Vehicle operates normally
 *   - Background task will restore primary bank later
 * 
 * \return This function does not return (system resets)
 */
void FlashBank_RollbackToGolden(void) __attribute__((noreturn));

/**
 * \brief Restore primary bank from golden (background task after rollback)
 * 
 * This function is called automatically when:
 *   - System booted from Golden bank (not primary)
 *   - Golden bank has been validated as stable
 * 
 * Process (Background - Takes time):
 *   1. Erase primary bank (Bank A)
 *   2. Copy Golden bank → Primary bank
 *   3. Verify copy
 *   4. Switch Boot Flag back to primary bank
 *   5. Perform system reset
 * 
 * After reset:
 *   - System boots from primary bank (Bank A)
 *   - Both banks now contain validated firmware
 *   - Normal operation restored
 * 
 * \return TRUE if restoration successful, FALSE otherwise
 */
boolean FlashBank_RestorePrimaryFromGolden(void);

/**
 * \brief Write OTA diagnostic log to OLDA memory (optional)
 * 
 * Stores OTA event logs in the OLDA region for debugging and diagnostics.
 * This is useful for post-mortem analysis of OTA failures.
 * 
 * \param logMessage Pointer to log message string
 * \param length Length of log message
 * \return TRUE if log write successful, FALSE otherwise
 */
boolean FlashBank_WriteOTALog(const char* logMessage, uint32 length);

/**
 * \brief Read OTA diagnostic log from OLDA memory (optional)
 * 
 * Retrieves OTA event logs from the OLDA region.
 * 
 * \param buffer Pointer to buffer to receive log data
 * \param maxLength Maximum length to read
 * \return Number of bytes read, 0 if no log available
 */
uint32 FlashBank_ReadOTALog(char* buffer, uint32 maxLength);

#ifdef DEBUG
/**
 * \brief Debug helper: Copy firmware from Bank A to Bank B for testing
 * 
 * This function is only available in debug builds.
 * Used when debugger downloads only to Bank A, but you want to test dual-bank.
 * 
 * Call this once during system initialization in debug mode.
 */
void FlashBank_DebugCopyAtoB(void);
#endif

#endif /* FLASH_BANK_MANAGER_H_ */

