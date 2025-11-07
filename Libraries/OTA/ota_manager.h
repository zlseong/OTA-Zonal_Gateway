/******************************************************************************
 * @file     ota_manager.h
 * @brief    OTA Package Manager with Dynamic Memory Allocation
 * @version  1.0.0
 * @date     2025-11-05
 * 
 * @details  Manages OTA firmware packages on external SPI Flash (S25FL512S)
 *           - Dynamic memory allocation (64MB Flash)
 *           - Package metadata table (up to 32 packages)
 *           - Garbage collection for deployed packages
 *           - Supports multiple target types (ZGW, CAN ECU, LIN ECU)
 *****************************************************************************/

#ifndef OTA_MANAGER_H
#define OTA_MANAGER_H

/******************************************************************************
 * Includes
 *****************************************************************************/
#include "Ifx_Types.h"
#include "../SPI_Flash/spi_flash_s25fl512s.h"

/******************************************************************************
 * Constants
 *****************************************************************************/

#define OTA_MAX_PACKAGES            32      /* Max simultaneous packages */
#define OTA_METADATA_ADDRESS        0x000000  /* Metadata sector (4KB) */
#define OTA_METADATA_SIZE           4096    /* 4KB for metadata */
#define OTA_DATA_START_ADDRESS      0x001000  /* Package data starts at 4KB */

#define OTA_MAGIC                   0x4F54414D  /* "OTAM" */
#define OTA_VERSION                 0x0100    /* Version 1.0 */

#define OTA_MAX_ECU_ID_LEN          16      /* Max ECU ID length */

/******************************************************************************
 * OTA Package Target Type
 *****************************************************************************/
typedef enum
{
    OTA_TARGET_ZGW          = 0x01,  /* Zonal Gateway itself */
    OTA_TARGET_CAN_ECU      = 0x02,  /* CAN-connected ECU */
    OTA_TARGET_LIN_ECU      = 0x03,  /* LIN-connected ECU */
    OTA_TARGET_ETH_ECU      = 0x04   /* Ethernet-connected ECU */
} OTA_TargetType;

/******************************************************************************
 * OTA Package Bus Type
 *****************************************************************************/
typedef enum
{
    OTA_BUS_INTERNAL        = 0x00,  /* Internal (for ZGW) */
    OTA_BUS_CAN_FD          = 0x01,  /* CAN FD */
    OTA_BUS_CAN_2_0         = 0x02,  /* CAN 2.0 */
    OTA_BUS_LIN             = 0x03,  /* LIN */
    OTA_BUS_ETHERNET        = 0x04   /* Ethernet/DoIP */
} OTA_BusType;

/******************************************************************************
 * OTA Deployment Condition
 *****************************************************************************/
typedef enum
{
    OTA_CONDITION_IMMEDIATE = 0x00,  /* Deploy immediately */
    OTA_CONDITION_IGN_OFF   = 0x01,  /* Deploy after ignition off */
    OTA_CONDITION_VEHICLE_STOP = 0x02, /* Deploy when vehicle stopped */
    OTA_CONDITION_MANUAL    = 0x03   /* Deploy on manual trigger */
} OTA_Condition;

/******************************************************************************
 * OTA Package Status
 *****************************************************************************/
typedef enum
{
    OTA_STATUS_EMPTY        = 0x00,  /* Slot is empty */
    OTA_STATUS_DOWNLOADING  = 0x01,  /* Download in progress */
    OTA_STATUS_COMPLETE     = 0x02,  /* Download complete */
    OTA_STATUS_PENDING      = 0x03,  /* Awaiting deployment */
    OTA_STATUS_DEPLOYING    = 0x04,  /* Deployment in progress */
    OTA_STATUS_DEPLOYED     = 0x05,  /* Successfully deployed (can be deleted) */
    OTA_STATUS_ERROR        = 0xFF   /* Error occurred */
} OTA_PackageStatus;

/******************************************************************************
 * OTA Package Entry (in Metadata Table)
 *****************************************************************************/
typedef struct
{
    uint32 flash_address;               /* SPI Flash start address */
    uint32 package_size;                /* Total package size (Header + Firmware) */
    
    char   target_ecu_id[OTA_MAX_ECU_ID_LEN]; /* Target ECU ID (e.g., "ECU_011") */
    uint8  target_type;                 /* OTA_TargetType */
    uint8  bus_type;                    /* OTA_BusType */
    uint8  condition;                   /* OTA_Condition */
    uint8  status;                      /* OTA_PackageStatus */
    
    uint32 timestamp;                   /* Package save timestamp */
    uint32 firmware_crc32;              /* Firmware CRC32 */
    
    uint8  reserved[12];                /* Future use */
    
} OTA_PackageEntry;  /* 64 bytes */

/******************************************************************************
 * OTA Manager Metadata (Stored in Flash at 0x000000)
 *****************************************************************************/
typedef struct
{
    uint32 magic;                       /* Magic number: 0x4F54414D ("OTAM") */
    uint16 version;                     /* Metadata version */
    uint16 package_count;               /* Current number of packages */
    
    uint32 free_space_start;            /* Next available flash address */
    uint32 free_space_size;             /* Remaining flash space */
    
    uint32 total_packages_received;     /* Lifetime statistics */
    uint32 total_bytes_received;
    
    uint8  reserved[40];                /* Future use */
    
    OTA_PackageEntry packages[OTA_MAX_PACKAGES];  /* Package table: 64 * 32 = 2048 bytes */
    
    uint8  padding[1984];               /* Pad to 4KB (4096 - 64 - 2048 = 1984) */
    
} OTA_Metadata;  /* Total: 4096 bytes (4KB) */

/******************************************************************************
 * OTA Manager Handle
 *****************************************************************************/
typedef struct
{
    SPI_Flash_S25FL512S *flash;         /* Pointer to SPI Flash handle */
    OTA_Metadata metadata;              /* In-memory metadata */
    boolean is_initialized;             /* Initialization flag */
    
} OTA_Manager;

/******************************************************************************
 * OTA Result Codes
 *****************************************************************************/
typedef enum
{
    OTA_OK = 0,
    OTA_ERROR_NOT_INITIALIZED,
    OTA_ERROR_INVALID_PARAMETER,
    OTA_ERROR_NO_SPACE,
    OTA_ERROR_PACKAGE_TABLE_FULL,
    OTA_ERROR_FLASH_ERROR,
    OTA_ERROR_PACKAGE_NOT_FOUND,
    OTA_ERROR_METADATA_CORRUPT,
    OTA_ERROR_MAGIC_MISMATCH
} OTA_Result;

/******************************************************************************
 * Function Prototypes
 *****************************************************************************/

/**
 * @brief Initialize OTA Manager
 * @param manager  Pointer to OTA Manager handle
 * @param flash    Pointer to initialized SPI Flash handle
 * @return OTA_OK on success
 * 
 * @note Reads metadata from flash. If not valid, initializes new metadata.
 */
OTA_Result OTA_Manager_Init(OTA_Manager *manager, SPI_Flash_S25FL512S *flash);

/**
 * @brief Allocate space for new OTA package
 * @param manager   Pointer to OTA Manager
 * @param size      Required size in bytes
 * @param address   Output: Allocated flash address
 * @return OTA_OK on success
 * 
 * @note Tries garbage collection if initial allocation fails
 */
OTA_Result OTA_Manager_AllocatePackage(OTA_Manager *manager, 
                                        uint32 size, 
                                        uint32 *address);

/**
 * @brief Add package entry to metadata table
 * @param manager       Pointer to OTA Manager
 * @param target_ecu_id Target ECU ID
 * @param target_type   OTA_TargetType
 * @param bus_type      OTA_BusType
 * @param condition     OTA_Condition
 * @param flash_addr    Flash address where package is stored
 * @param package_size  Total package size
 * @param firmware_crc  Firmware CRC32
 * @return OTA_OK on success
 */
OTA_Result OTA_Manager_AddPackage(OTA_Manager *manager,
                                   const char *target_ecu_id,
                                   uint8 target_type,
                                   uint8 bus_type,
                                   uint8 condition,
                                   uint32 flash_addr,
                                   uint32 package_size,
                                   uint32 firmware_crc);

/**
 * @brief Find package by ECU ID
 * @param manager       Pointer to OTA Manager
 * @param target_ecu_id Target ECU ID
 * @param entry_out     Output: Package entry (if found)
 * @return OTA_OK if found, OTA_ERROR_PACKAGE_NOT_FOUND otherwise
 */
OTA_Result OTA_Manager_FindPackage(OTA_Manager *manager,
                                    const char *target_ecu_id,
                                    OTA_PackageEntry *entry_out);

/**
 * @brief Update package status
 * @param manager       Pointer to OTA Manager
 * @param target_ecu_id Target ECU ID
 * @param new_status    New status
 * @return OTA_OK on success
 */
OTA_Result OTA_Manager_UpdatePackageStatus(OTA_Manager *manager,
                                            const char *target_ecu_id,
                                            OTA_PackageStatus new_status);

/**
 * @brief Delete package (mark slot as empty)
 * @param manager       Pointer to OTA Manager
 * @param target_ecu_id Target ECU ID
 * @return OTA_OK on success
 * 
 * @note Does not erase flash data, only marks slot as empty
 */
OTA_Result OTA_Manager_DeletePackage(OTA_Manager *manager,
                                      const char *target_ecu_id);

/**
 * @brief Garbage Collection - Remove deployed packages
 * @param manager  Pointer to OTA Manager
 * @return OTA_OK on success
 * 
 * @note Removes all packages with status OTA_STATUS_DEPLOYED
 */
OTA_Result OTA_Manager_GarbageCollect(OTA_Manager *manager);

/**
 * @brief Save metadata to flash
 * @param manager  Pointer to OTA Manager
 * @return OTA_OK on success
 */
OTA_Result OTA_Manager_SaveMetadata(OTA_Manager *manager);

/**
 * @brief Get statistics
 * @param manager       Pointer to OTA Manager
 * @param package_count Output: Current package count
 * @param free_space    Output: Available space in bytes
 * @param total_received Output: Lifetime total packages
 */
void OTA_Manager_GetStatistics(OTA_Manager *manager,
                                uint16 *package_count,
                                uint32 *free_space,
                                uint32 *total_received);

/**
 * @brief Get error string
 * @param result  OTA result code
 * @return Human-readable error message
 */
const char* OTA_Manager_GetErrorString(OTA_Result result);

#endif /* OTA_MANAGER_H */

