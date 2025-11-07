/******************************************************************************
 * @file     spi_flash_s25fl512s.h
 * @brief    S25FL512S (64MB) SPI Flash Driver for MIKROE-3191 (Flash Click)
 * @version  1.0.0
 * @date     2025-11-05
 * 
 * @details  Driver for Cypress/Infineon S25FL512S 512Mbit (64MB) SPI Flash
 *           - JEDEC ID: 0x01 0x02 0x20
 *           - Capacity: 64MB (512Mbit)
 *           - Page Size: 256 bytes
 *           - Sector Size: 4KB / 64KB / 256KB (Hybrid)
 *           - Interface: SPI / Quad SPI
 * 
 * Hardware: MIKROE-3191 Flash Click (mikroBUS)
 *           - CS:   P20.8  (QSPI3_SLSO0)
 *           - SCLK: P20.11 (QSPI3_SCLK)
 *           - MOSI: P20.14 (QSPI3_MTSR)
 *           - MISO: P20.12 (QSPI3_MRST)
 *****************************************************************************/

#ifndef SPI_FLASH_S25FL512S_H
#define SPI_FLASH_S25FL512S_H

/******************************************************************************
 * Includes
 *****************************************************************************/
#include "Ifx_Types.h"
#include "IfxQspi_SpiMaster.h"

/******************************************************************************
 * S25FL512S Command Set
 *****************************************************************************/

/* Basic Commands */
#define S25FL512S_CMD_READ_ID               0x9F    /* Read JEDEC ID (3 bytes) */
#define S25FL512S_CMD_READ_MANU_DEV_ID      0x90    /* Read Manufacturer/Device ID */
#define S25FL512S_CMD_READ_STATUS_REG1      0x05    /* Read Status Register 1 */
#define S25FL512S_CMD_READ_STATUS_REG2      0x07    /* Read Status Register 2 */
#define S25FL512S_CMD_WRITE_ENABLE          0x06    /* Write Enable */
#define S25FL512S_CMD_WRITE_DISABLE         0x04    /* Write Disable */
#define S25FL512S_CMD_WRITE_STATUS_REG      0x01    /* Write Status Register 1 */

/* Read Commands */
#define S25FL512S_CMD_READ_DATA             0x03    /* Read Data (standard, up to 50MHz) */
#define S25FL512S_CMD_FAST_READ             0x0B    /* Fast Read (with dummy byte, up to 133MHz) */
#define S25FL512S_CMD_READ_4BYTE            0x13    /* 4-byte address Read */

/* Write Commands */
#define S25FL512S_CMD_PAGE_PROGRAM          0x02    /* Page Program (256 bytes) */
#define S25FL512S_CMD_PAGE_PROGRAM_4BYTE    0x12    /* 4-byte address Page Program */

/* Erase Commands */
#define S25FL512S_CMD_SECTOR_ERASE_4KB      0x20    /* 4KB Sector Erase (P4E) */
#define S25FL512S_CMD_SECTOR_ERASE_4KB_4B   0x21    /* 4KB Sector Erase (4-byte addr) */
#define S25FL512S_CMD_BLOCK_ERASE_64KB      0xD8    /* 64KB Block Erase */
#define S25FL512S_CMD_BLOCK_ERASE_256KB     0xDC    /* 256KB Block Erase */
#define S25FL512S_CMD_CHIP_ERASE            0x60    /* Chip Erase (Full) */
#define S25FL512S_CMD_CHIP_ERASE_ALT        0xC7    /* Chip Erase (Alternative) */

/* Special Commands */
#define S25FL512S_CMD_DEEP_POWER_DOWN       0xB9    /* Enter Deep Power Down */
#define S25FL512S_CMD_RELEASE_POWER_DOWN    0xAB    /* Release from Deep Power Down */
#define S25FL512S_CMD_RESET_ENABLE          0x66    /* Software Reset Enable */
#define S25FL512S_CMD_RESET                 0x99    /* Software Reset */
#define S25FL512S_CMD_EXIT_4BYTE_MODE       0xE9    /* Exit 4-byte Address Mode */
#define S25FL512S_CMD_ENTER_4BYTE_MODE      0xB7    /* Enter 4-byte Address Mode */

/******************************************************************************
 * S25FL512S Status Register Bits
 *****************************************************************************/

/* Status Register 1 (SR1) */
#define S25FL512S_SR1_WIP                   (1 << 0)  /* Write In Progress */
#define S25FL512S_SR1_WEL                   (1 << 1)  /* Write Enable Latch */
#define S25FL512S_SR1_BP0                   (1 << 2)  /* Block Protect bit 0 */
#define S25FL512S_SR1_BP1                   (1 << 3)  /* Block Protect bit 1 */
#define S25FL512S_SR1_BP2                   (1 << 4)  /* Block Protect bit 2 */
#define S25FL512S_SR1_BP3                   (1 << 5)  /* Block Protect bit 3 */
#define S25FL512S_SR1_BP4                   (1 << 6)  /* Block Protect bit 4 */
#define S25FL512S_SR1_SRP                   (1 << 7)  /* Status Register Protect */

/******************************************************************************
 * S25FL512S Device Information
 *****************************************************************************/

#define S25FL512S_JEDEC_ID_MANUFACTURER     0x01    /* Cypress/Infineon */
#define S25FL512S_JEDEC_ID_MEMORY_TYPE      0x02    /* S25FL-S Family */
#define S25FL512S_JEDEC_ID_CAPACITY         0x20    /* 512Mbit (64MB) */

#define S25FL512S_CAPACITY_BYTES            (64 * 1024 * 1024)  /* 64MB */
#define S25FL512S_PAGE_SIZE                 256                 /* 256 bytes */
#define S25FL512S_SECTOR_SIZE_4KB           (4 * 1024)          /* 4KB */
#define S25FL512S_BLOCK_SIZE_64KB           (64 * 1024)         /* 64KB */
#define S25FL512S_BLOCK_SIZE_256KB          (256 * 1024)        /* 256KB */

#define S25FL512S_PAGES_PER_SECTOR_4KB      (S25FL512S_SECTOR_SIZE_4KB / S25FL512S_PAGE_SIZE)      /* 16 */
#define S25FL512S_SECTORS_PER_BLOCK_64KB    (S25FL512S_BLOCK_SIZE_64KB / S25FL512S_SECTOR_SIZE_4KB) /* 16 */

/* Timing Parameters (typical values in milliseconds) */
#define S25FL512S_TIMEOUT_PAGE_PROGRAM_MS   3       /* Max 3ms for page program */
#define S25FL512S_TIMEOUT_SECTOR_ERASE_MS   400     /* Max 400ms for 4KB sector erase */
#define S25FL512S_TIMEOUT_BLOCK_ERASE_MS    3000    /* Max 3000ms for 64KB block erase */
#define S25FL512S_TIMEOUT_CHIP_ERASE_MS     (400 * 1000)  /* Max 400s for chip erase */

/******************************************************************************
 * Data Structures
 *****************************************************************************/

/**
 * @brief S25FL512S Flash Handle
 */
typedef struct
{
    IfxQspi_SpiMaster         qspi;           /* QSPI Master handle */
    IfxQspi_SpiMaster_Channel channel;        /* QSPI Master Channel handle */
    
    uint8  manufacturer_id;                   /* JEDEC Manufacturer ID (0x01) */
    uint8  memory_type;                       /* JEDEC Memory Type (0x02) */
    uint8  capacity;                          /* JEDEC Capacity Code (0x20) */
    
    boolean is_initialized;                   /* Initialization status */
    
} SPI_Flash_S25FL512S;

/**
 * @brief Flash Operation Result
 */
typedef enum
{
    FLASH_OK = 0,
    FLASH_ERROR_NOT_INITIALIZED,
    FLASH_ERROR_INVALID_PARAMETER,
    FLASH_ERROR_TIMEOUT,
    FLASH_ERROR_WRITE_PROTECTED,
    FLASH_ERROR_WRITE_ENABLE_FAILED,
    FLASH_ERROR_ERASE_FAILED,
    FLASH_ERROR_PROGRAM_FAILED,
    FLASH_ERROR_VERIFY_FAILED,
    FLASH_ERROR_BUSY,
    FLASH_ERROR_JEDEC_ID_MISMATCH
} SPI_Flash_Result;

/******************************************************************************
 * Function Prototypes
 *****************************************************************************/

/**
 * @brief Initialize S25FL512S Flash
 * @param handle  Pointer to flash handle
 * @return FLASH_OK on success, error code otherwise
 */
SPI_Flash_Result SPI_Flash_Init(SPI_Flash_S25FL512S *handle);

/**
 * @brief Read JEDEC ID (Manufacturer, Type, Capacity)
 * @param handle  Pointer to flash handle
 * @param manufacturer  Output: Manufacturer ID (0x01 for Cypress)
 * @param memory_type   Output: Memory Type (0x02 for S25FL-S)
 * @param capacity      Output: Capacity (0x20 for 512Mbit)
 * @return FLASH_OK on success
 */
SPI_Flash_Result SPI_Flash_ReadID(SPI_Flash_S25FL512S *handle, 
                                   uint8 *manufacturer, 
                                   uint8 *memory_type, 
                                   uint8 *capacity);

/**
 * @brief Read Status Register 1
 * @param handle  Pointer to flash handle
 * @param status  Output: Status register value
 * @return FLASH_OK on success
 */
SPI_Flash_Result SPI_Flash_ReadStatus(SPI_Flash_S25FL512S *handle, uint8 *status);

/**
 * @brief Check if flash is busy (Write In Progress)
 * @param handle  Pointer to flash handle
 * @return TRUE if busy, FALSE if ready
 */
boolean SPI_Flash_IsBusy(SPI_Flash_S25FL512S *handle);

/**
 * @brief Wait until flash is ready (not busy)
 * @param handle      Pointer to flash handle
 * @param timeout_ms  Timeout in milliseconds
 * @return FLASH_OK if ready, FLASH_ERROR_TIMEOUT on timeout
 */
SPI_Flash_Result SPI_Flash_WaitReady(SPI_Flash_S25FL512S *handle, uint32 timeout_ms);

/**
 * @brief Enable write operations (must be called before program/erase)
 * @param handle  Pointer to flash handle
 * @return FLASH_OK on success
 */
SPI_Flash_Result SPI_Flash_WriteEnable(SPI_Flash_S25FL512S *handle);

/**
 * @brief Disable write operations
 * @param handle  Pointer to flash handle
 * @return FLASH_OK on success
 */
SPI_Flash_Result SPI_Flash_WriteDisable(SPI_Flash_S25FL512S *handle);

/**
 * @brief Read data from flash
 * @param handle  Pointer to flash handle
 * @param address Flash address (0x000000 - 0x3FFFFFF for 64MB)
 * @param buffer  Output buffer
 * @param length  Number of bytes to read
 * @return FLASH_OK on success
 */
SPI_Flash_Result SPI_Flash_Read(SPI_Flash_S25FL512S *handle, 
                                 uint32 address, 
                                 uint8 *buffer, 
                                 uint32 length);

/**
 * @brief Write data to flash (page program)
 * @param handle  Pointer to flash handle
 * @param address Flash address (must be aligned to page boundary for best performance)
 * @param data    Data to write
 * @param length  Number of bytes to write (max 256 bytes per call)
 * @return FLASH_OK on success
 * 
 * @note This function handles page boundary crossing automatically
 * @note Write Enable is called internally
 * @note Function waits for completion
 */
SPI_Flash_Result SPI_Flash_Write(SPI_Flash_S25FL512S *handle, 
                                  uint32 address, 
                                  const uint8 *data, 
                                  uint32 length);

/**
 * @brief Erase 4KB sector
 * @param handle  Pointer to flash handle
 * @param address Sector address (must be aligned to 4KB boundary)
 * @return FLASH_OK on success
 * 
 * @note Write Enable is called internally
 * @note Function waits for completion (max 400ms)
 */
SPI_Flash_Result SPI_Flash_EraseSector(SPI_Flash_S25FL512S *handle, uint32 address);

/**
 * @brief Erase 64KB block
 * @param handle  Pointer to flash handle
 * @param address Block address (must be aligned to 64KB boundary)
 * @return FLASH_OK on success
 * 
 * @note Write Enable is called internally
 * @note Function waits for completion (max 3000ms)
 */
SPI_Flash_Result SPI_Flash_EraseBlock(SPI_Flash_S25FL512S *handle, uint32 address);

/**
 * @brief Erase entire chip
 * @param handle  Pointer to flash handle
 * @return FLASH_OK on success
 * 
 * @note Write Enable is called internally
 * @note Function waits for completion (max 400 seconds!)
 * @warning This erases the entire 64MB flash!
 */
SPI_Flash_Result SPI_Flash_EraseChip(SPI_Flash_S25FL512S *handle);

/**
 * @brief Software reset
 * @param handle  Pointer to flash handle
 * @return FLASH_OK on success
 */
SPI_Flash_Result SPI_Flash_Reset(SPI_Flash_S25FL512S *handle);

/**
 * @brief Get error message string
 * @param result  Flash operation result code
 * @return Human-readable error message
 */
const char* SPI_Flash_GetErrorString(SPI_Flash_Result result);

#endif /* SPI_FLASH_S25FL512S_H */

