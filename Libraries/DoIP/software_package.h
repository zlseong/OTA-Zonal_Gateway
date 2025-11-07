/*******************************************************************************
 * @file    software_package.h
 * @brief   Software Package Header Definition for OTA Updates
 * @details Defines package format with target ECU routing information
 * 
 * @version 1.0
 * @date    2025-11-06
 ******************************************************************************/

#ifndef SOFTWARE_PACKAGE_H
#define SOFTWARE_PACKAGE_H

#include "Ifx_Types.h"

/*******************************************************************************
 * Software Package Format
 ******************************************************************************/

/* Package Header Magic */
#define SW_PKG_MAGIC                0x53575047  /* "SWPG" */

/* Target ECU IDs */
#define ECU_ID_INVALID              0x0000
#define ECU_ID_ZGW                  0x0091      /* ECU_091 - Zonal Gateway */
#define ECU_ID_ZONE_1               0x0011      /* ECU_011 - Zone ECU 1 */
#define ECU_ID_ZONE_2               0x0012      /* ECU_012 - Zone ECU 2 */
#define ECU_ID_ZONE_3               0x0013      /* ECU_013 - Zone ECU 3 */
#define ECU_ID_BROADCAST            0xFFFF      /* All ECUs */

/* Software Types */
#define SW_TYPE_APPLICATION         0x01
#define SW_TYPE_BOOTLOADER          0x02
#define SW_TYPE_CALIBRATION         0x03
#define SW_TYPE_CONFIGURATION       0x04

/* MCU Flash Bank Selection (for ZGW self-update only) */
typedef enum
{
    MCU_BANK_A = 0,
    MCU_BANK_B = 1,
    MCU_BANK_UNKNOWN = 0xFF
} MCU_FlashBank;

/*******************************************************************************
 * Software Package Header Structure (64 bytes)
 ******************************************************************************/

/* TASKING compiler: Use __packed__ attribute instead of #pragma align 1 */
typedef struct __attribute__((packed))
{
    /* Identification (16 bytes) */
    uint32 magic;                   /* Magic number: 0x53575047 ("SWPG") */
    uint16 target_ecu_id;           /* Target ECU ID (ECU_011, ECU_091, etc.) */
    uint8  software_type;           /* Software type (APP, BOOT, CAL, CFG) */
    uint8  compression;             /* Compression method (0=none, 1=gzip) */
    uint32 payload_size;            /* Payload size (bytes) */
    uint32 uncompressed_size;       /* Uncompressed size (if compressed) */
    
    /* Version Information (12 bytes) */
    uint8  version_major;           /* Major version (e.g., 1.x.x) */
    uint8  version_minor;           /* Minor version (e.g., x.2.x) */
    uint8  version_patch;           /* Patch version (e.g., x.x.3) */
    uint8  version_build;           /* Build number */
    uint32 version_timestamp;       /* Unix timestamp */
    uint32 version_serial;          /* Serial number */
    
    /* Security & Integrity (16 bytes) */
    uint32 crc32;                   /* CRC32 of payload */
    uint32 signature[3];            /* Digital signature (reserved) */
    
    /* Routing Information (8 bytes) */
    uint16 source_ecu_id;           /* Source ECU (e.g., VMG=0x0E00) */
    uint16 hop_count;               /* Hop count (for multi-hop routing) */
    uint32 sequence_number;         /* Sequence number (for multi-package) */
    
    /* Reserved (12 bytes) */
    uint8  reserved[12];            /* Reserved for future use */
    
} SoftwarePackageHeader;

/* Header size validation - TASKING doesn't support _Static_assert in C99 mode */
/* Verify manually: sizeof(SoftwarePackageHeader) should be 64 bytes */

/*******************************************************************************
 * SPI Flash Staging Areas (Temporary Buffer for all ECUs)
 ******************************************************************************/

/* SPI Flash는 임시 버퍼 - 여러 ECU의 소프트웨어를 동시에 저장 가능 */
#define SPI_FLASH_TOTAL_SIZE        0x04000000  /* 64MB */

/* ECU_091 (Zonal Gateway) Staging Area */
#define SPI_FLASH_ECU_091_START     0x00000000  /* 4MB for ZGW */
#define SPI_FLASH_ECU_091_SIZE      0x00400000

/* ECU_011 (Zone ECU 1) Staging Area */
#define SPI_FLASH_ECU_011_START     0x00400000  /* 4MB for Zone 1 */
#define SPI_FLASH_ECU_011_SIZE      0x00400000

/* ECU_012 (Zone ECU 2) Staging Area */
#define SPI_FLASH_ECU_012_START     0x00800000  /* 4MB for Zone 2 */
#define SPI_FLASH_ECU_012_SIZE      0x00400000

/* ECU_013 (Zone ECU 3) Staging Area */
#define SPI_FLASH_ECU_013_START     0x00C00000  /* 4MB for Zone 3 */
#define SPI_FLASH_ECU_013_SIZE      0x00400000

/* Reserved for future ECUs */
#define SPI_FLASH_RESERVED_START    0x01000000  /* 48MB reserved */

/*******************************************************************************
 * Software Metadata Storage (in MCU PFLASH)
 ******************************************************************************/

/* Software Metadata Location in MCU PFLASH */
#define MCU_SW_METADATA_OFFSET      0x00000100  /* 256 bytes after reset vector */
#define MCU_SW_METADATA_SIZE        256         /* Reserved space for metadata */

/* Active Bank Metadata Address */
#define MCU_SW_METADATA_BANK_A      (MCU_PFLASH_BANK_A_START + MCU_SW_METADATA_OFFSET)
#define MCU_SW_METADATA_BANK_B      (MCU_PFLASH_BANK_B_START + MCU_SW_METADATA_OFFSET)

/*******************************************************************************
 * MCU PFLASH Dual Bank Configuration (ZGW self-update only)
 ******************************************************************************/

/* MCU 내장 Flash - Dual Bank for safe ZGW self-updates */
/* NOTE: 실제 주소는 Linker Script (LSL)에 정의됨 */
#define MCU_PFLASH_BANK_A_START     0xA0000000  /* 2MB - Bank A */
#define MCU_PFLASH_BANK_A_SIZE      0x00200000

#define MCU_PFLASH_BANK_B_START     0xA0200000  /* 2MB - Bank B */
#define MCU_PFLASH_BANK_B_SIZE      0x00200000

/* Bank Marker는 MCU Data Flash (DFLASH)에 저장 */
#define MCU_DFLASH_BANK_MARKER      0xAF000000  /* DFLASH address */
#define MCU_BANK_A_MARKER           0xAAAAAAAA
#define MCU_BANK_B_MARKER           0xBBBBBBBB

/*******************************************************************************
 * MCU Flash Bank Manager (ZGW self-update only)
 ******************************************************************************/

typedef struct
{
    MCU_FlashBank active_bank;      /* Currently active MCU bank */
    MCU_FlashBank standby_bank;     /* Standby MCU bank (for updates) */
    uint32 active_start_address;    /* Active MCU bank start address */
    uint32 standby_start_address;   /* Standby MCU bank start address */
    boolean bank_switch_pending;    /* Bank switch pending flag */
} MCU_FlashBankManager;

/*******************************************************************************
 * Public Functions
 ******************************************************************************/

/**
 * @brief Initialize MCU Flash Bank Manager
 * @return TRUE if successful, FALSE otherwise
 */
boolean MCU_FlashBank_Init(void);

/**
 * @brief Get active MCU flash bank
 * @return Active bank (A or B)
 */
MCU_FlashBank MCU_FlashBank_GetActive(void);

/**
 * @brief Get standby MCU flash bank
 * @return Standby bank (A or B)
 */
MCU_FlashBank MCU_FlashBank_GetStandby(void);

/**
 * @brief Get MCU flash address for bank
 * @param bank MCU Flash bank (A or B)
 * @return MCU Flash start address
 */
uint32 MCU_FlashBank_GetAddress(MCU_FlashBank bank);

/**
 * @brief Request bootloader to perform update (sets flag and reboots)
 * @param spi_source_address SPI Flash staging area address
 * @param size Software size in bytes
 * @return TRUE if request successful, FALSE otherwise
 */
boolean MCU_FlashBank_RequestUpdate(uint32 spi_source_address, uint32 size);

/**
 * @brief Parse software package header
 * @param data Raw data buffer
 * @param header Output header structure
 * @return TRUE if valid, FALSE if invalid
 */
boolean SoftwarePackage_ParseHeader(const uint8 *data, SoftwarePackageHeader *header);

/**
 * @brief Verify package header integrity
 * @param header Package header
 * @return TRUE if valid, FALSE if invalid
 */
boolean SoftwarePackage_VerifyHeader(const SoftwarePackageHeader *header);

/**
 * @brief Calculate CRC32 of payload
 * @param data Payload data
 * @param length Payload length
 * @return CRC32 value
 */
uint32 SoftwarePackage_CalculateCRC32(const uint8 *data, uint32 length);

/**
 * @brief Check if package is for this ECU (ZGW)
 * @param header Package header
 * @return TRUE if target is ZGW, FALSE otherwise
 */
boolean SoftwarePackage_IsForThisECU(const SoftwarePackageHeader *header);

/**
 * @brief Get target ECU name string
 * @param ecu_id ECU ID
 * @return ECU name string
 */
const char* SoftwarePackage_GetECUName(uint16 ecu_id);

/**
 * @brief Read software metadata from active MCU bank
 * @param metadata Output: Software metadata (SoftwarePackageHeader)
 * @return TRUE if valid metadata found, FALSE otherwise
 * 
 * @note This function reads the SoftwarePackageHeader stored at offset 0x100
 *       in the currently active MCU PFLASH bank. Used during boot to determine
 *       the current software version for VCI reporting.
 */
boolean SoftwarePackage_ReadActiveMetadata(SoftwarePackageHeader *metadata);

#endif /* SOFTWARE_PACKAGE_H */

