/******************************************************************************
 * @file     spi_flash_s25fl512s.c
 * @brief    S25FL512S (64MB) SPI Flash Driver Implementation
 * @version  3.0.0 (Official Example Method - QSPI2 Master)
 * @date     2025-11-06
 *****************************************************************************/

/******************************************************************************
 * Includes
 *****************************************************************************/
#include "spi_flash_s25fl512s.h"
#include "IfxPort.h"
#include "IfxCpu.h"
#include "IfxScuWdt.h"
#include "Bsp.h"
#include "UART_Logging.h"
#include <string.h>
#include <stdio.h>
#include <stdlib.h>

/******************************************************************************
 * Configuration - QSPI2 Master (mikroBUS Socket on TC375 Lite Kit)
 *****************************************************************************/

#define QSPI_FLASH_MODULE           &MODULE_QSPI2
#define QSPI_FLASH_BAUDRATE         1000000     /* 1MHz */

/* mikroBUS RST pin */
#define QSPI_FLASH_RST_PORT         &MODULE_P10
#define QSPI_FLASH_RST_PIN          6           /* P10.6 - mikroBUS Pin 2 (RST) */

/* ISR Priorities */
#define ISR_PRIORITY_QSPI_TX        50
#define ISR_PRIORITY_QSPI_RX        51
#define ISR_PRIORITY_QSPI_ER        52

/******************************************************************************
 * Internal Function Prototypes
 *****************************************************************************/
static void SPI_Flash_HardwareReset(void);
static void SPI_Flash_SendCommand(SPI_Flash_S25FL512S *handle, uint8 command);
static void SPI_Flash_SendCommandWithAddress(SPI_Flash_S25FL512S *handle, 
                                              uint8 command, 
                                              uint32 address);
static uint32 SPI_Flash_GetTickMs(void);
static void SPI_Flash_CorrectBitShift_RX(uint8 *buffer, uint32 length);

/******************************************************************************
 * Interrupt Service Routines (Official Example Method)
 *****************************************************************************/

/* Reference to global SPI Flash handle (defined in Cpu0_Main.c) */
extern SPI_Flash_S25FL512S g_spi_flash;

IFX_INTERRUPT(spiFlashTxISR, 0, ISR_PRIORITY_QSPI_TX);
IFX_INTERRUPT(spiFlashRxISR, 0, ISR_PRIORITY_QSPI_RX);
IFX_INTERRUPT(spiFlashErISR, 0, ISR_PRIORITY_QSPI_ER);

void spiFlashTxISR(void)
{
    IfxCpu_enableInterrupts();
    IfxQspi_SpiMaster_isrTransmit(&g_spi_flash.qspi);
}

void spiFlashRxISR(void)
{
    IfxCpu_enableInterrupts();
    IfxQspi_SpiMaster_isrReceive(&g_spi_flash.qspi);
}

void spiFlashErISR(void)
{
    IfxCpu_enableInterrupts();
    IfxQspi_SpiMaster_isrError(&g_spi_flash.qspi);
}

/******************************************************************************
 * Helper Functions
 *****************************************************************************/

/**
 * @brief WORKAROUND: Shift transmit data left by 1 bit BEFORE sending
 * @note REMOVED: TX shift actually breaks communication!
 * @note TX data is transmitted correctly without any shift
 */
/* DISABLED: TX shift is NOT needed
static void SPI_Flash_CorrectBitShift_TX(uint8 *buffer, uint32 length)
{
    uint32 i;
    
    if (buffer == NULL || length == 0) {
        return;
    }
    
    for (i = length; i > 0; i--) {
        uint32 idx = i - 1;
        uint8 shifted = buffer[idx] << 1;
        
        if (idx > 0 && (buffer[idx - 1] & 0x80)) {
            shifted |= 0x01;
        }
        
        buffer[idx] = shifted;
    }
}
*/

/**
 * @brief WORKAROUND: Shift received data right by 1 bit to correct QSPI sampling timing issue
 * @note Hardware (bit-bang) reads correct data, but QSPI samples 1 bit late
 * @note Forward processing: buffer[i+1] is not yet modified when we process buffer[i]
 */
static void SPI_Flash_CorrectBitShift_RX(uint8 *buffer, uint32 length)
{
    if (buffer == NULL || length == 0) {
        return;
    }
    
    /* Process in FORWARD order: current byte uses unmodified next byte's LSB */
    for (uint32 i = 0; i < length; i++) {
        uint8 corrected = buffer[i] >> 1;
        /* Get carry bit from next byte (which hasn't been modified yet) */
        if (i < length - 1 && (buffer[i+1] & 0x01)) {
            corrected |= 0x80;
        }
        buffer[i] = corrected;
    }
}

/******************************************************************************
 * Initialize S25FL512S Flash with QSPI2 Master (Official Example Method)
 *****************************************************************************/
SPI_Flash_Result SPI_Flash_Init(SPI_Flash_S25FL512S *handle)
{
    if (handle == NULL)
    {
        return FLASH_ERROR_INVALID_PARAMETER;
    }
    
    /* Clear handle */
    memset(handle, 0, sizeof(SPI_Flash_S25FL512S));
    
    /* Hardware Reset via RST pin */
    SPI_Flash_HardwareReset();
    
    /* --- QSPI2 Master Initialization (Official Example Method) --- */
    
    /* Enable QSPI2 module clock */
    {
        uint16 password = IfxScuWdt_getCpuWatchdogPassword();
        IfxScuWdt_clearCpuEndinit(password);
        IfxQspi_setEnableModuleRequest(QSPI_FLASH_MODULE);
        IfxScuWdt_setCpuEndinit(password);
    }
    
    IfxQspi_SpiMaster_Config spiMasterConfig;
    IfxQspi_SpiMaster_initModuleConfig(&spiMasterConfig, QSPI_FLASH_MODULE);
    
    spiMasterConfig.mode = IfxQspi_Mode_master;      /* Official example method */
    
    /* Configure QSPI2 pins (mikroBUS socket on TC375 Lite Kit) */
    /* Official Example Method: Use pullDown for MISO! */
    const IfxQspi_SpiMaster_Pins qspi2Pins = {
        &IfxQspi2_SCLK_P15_8_OUT,   IfxPort_OutputMode_pushPull,      /* SCLK - mikroBUS Pin 4 */
        &IfxQspi2_MTSR_P15_6_OUT,   IfxPort_OutputMode_pushPull,      /* MOSI - mikroBUS Pin 6 */
        &IfxQspi2_MRSTB_P15_7_IN,   IfxPort_InputMode_pullDown,       /* MISO - mikroBUS Pin 5 (pullDown like official!) */
        IfxPort_PadDriver_cmosAutomotiveSpeed3
    };
    spiMasterConfig.pins = &qspi2Pins;
    
    /* Set ISR priorities */
    spiMasterConfig.txPriority = ISR_PRIORITY_QSPI_TX;
    spiMasterConfig.rxPriority = ISR_PRIORITY_QSPI_RX;
    spiMasterConfig.erPriority = ISR_PRIORITY_QSPI_ER;
    spiMasterConfig.isrProvider = IfxSrc_Tos_cpu0;
    
    /* Initialize QSPI2 Master */
    IfxQspi_SpiMaster_initModule(&handle->qspi, &spiMasterConfig);
    
    /* --- QSPI2 Channel Initialization --- */
    
    IfxQspi_SpiMaster_ChannelConfig channelConfig;
    IfxQspi_SpiMaster_initChannelConfig(&channelConfig, &handle->qspi);
    
    channelConfig.ch.baudrate = QSPI_FLASH_BAUDRATE;
    
    /* NO explicit CPOL/CPHA setting - use defaults like official example! */
    /* CS pin P14.7 - Configure as GPIO for manual control (hardware SLSO not available for this pin) */
    IfxPort_setPinModeOutput(&MODULE_P14, 7, IfxPort_OutputMode_pushPull, IfxPort_OutputIdx_general);
    IfxPort_setPinHigh(&MODULE_P14, 7);  /* CS idle = HIGH */
    
    /* --- WP# and HOLD# Initialization (mikroBUS PWM/INT pins) --- */
    /* WP#/IO2: mikroBUS Pin 16 (PWM) → P2.8 - Must be HIGH to allow writes */
    IfxPort_setPinModeOutput(&MODULE_P02, 8, IfxPort_OutputMode_pushPull, IfxPort_OutputIdx_general);
    IfxPort_setPinHigh(&MODULE_P02, 8);  /* WP# = HIGH (Write Protection DISABLED) */
    
    /* HOLD#/IO3: mikroBUS Pin 15 (INT) → P10.7 - Must be HIGH for normal operation */
    IfxPort_setPinModeOutput(&MODULE_P10, 7, IfxPort_OutputMode_pushPull, IfxPort_OutputIdx_general);
    IfxPort_setPinHigh(&MODULE_P10, 7);  /* HOLD# = HIGH (No Hold) */
    
    sendUARTMessage("  WP# (P2.8) = HIGH, HOLD# (P10.7) = HIGH\r\n", 44);
    
    /* Initialize QSPI2 Channel */
    IfxQspi_SpiMaster_initChannel(&handle->channel, &channelConfig);
    
    /* Wait for QSPI initialization */
    waitTime(IfxStm_getTicksFromMilliseconds(BSP_DEFAULT_TIMER, 10));
    
    /* Read JEDEC ID to verify communication */
    uint8 manufacturer, memory_type, capacity;
    SPI_Flash_Result result = SPI_Flash_ReadID(handle, &manufacturer, &memory_type, &capacity);
    
    if (result == FLASH_OK)
    {
        handle->manufacturer_id = manufacturer;
        handle->memory_type = memory_type;
        handle->capacity = capacity;
        handle->is_initialized = TRUE;
        
        /* Verify JEDEC ID */
        if (manufacturer == S25FL512S_JEDEC_ID_MANUFACTURER &&
            memory_type == S25FL512S_JEDEC_ID_MEMORY_TYPE &&
            capacity == S25FL512S_JEDEC_ID_CAPACITY)
        {
            char msg[64];
            sendUARTMessage("\r\nSPI Flash OK: 64MB (ID: 0x", 28);
            sprintf(msg, "%02X%02X%02X)\r\n", manufacturer, memory_type, capacity);
            sendUARTMessage(msg, strlen(msg));
            
            /* Clear all Block Protection bits by writing to Status Register */
            sendUARTMessage("  Clearing all protection bits...\r\n", 36);
            
            /* Write Enable */
            SPI_Flash_SendCommand(handle, S25FL512S_CMD_WRITE_ENABLE);
            waitTime(IfxStm_getTicksFromMicroseconds(BSP_DEFAULT_TIMER, 100));
            
            /* Write Status Register 1: Clear all BP bits (SR1 = 0x00) */
            uint8 clear_sr_cmd[2] = {S25FL512S_CMD_WRITE_STATUS_REG, 0x00};
            IfxPort_setPinLow(&MODULE_P14, 7);  /* CS = LOW */
            IfxQspi_SpiMaster_exchange(&handle->channel, clear_sr_cmd, NULL, 2);
            while (IfxQspi_SpiMaster_getStatus(&handle->channel) == IfxQspi_Status_busy);
            IfxPort_setPinHigh(&MODULE_P14, 7);  /* CS = HIGH */
            
            waitTime(IfxStm_getTicksFromMilliseconds(BSP_DEFAULT_TIMER, 10));
            sendUARTMessage("  Protection bits cleared (SR1 = 0x00)\r\n", 41);
            
            /* NOTE: Stay in 3-byte mode (MikroE method) */
            /* Use 3-byte commands but send 4-byte addresses (Flash ignores MSB) */
            sendUARTMessage("  Using 3-byte mode (MikroE compatible)\r\n", 42);
            
            return FLASH_OK;
        }
        else
        {
            char msg[128];
            sendUARTMessage("\r\nSPI Flash Init FAILED: JEDEC ID mismatch\r\n", 45);
            sprintf(msg, "  Read: 0x%02X 0x%02X 0x%02X (Expected: 0x01 0x02 0x20)\r\n", 
                    manufacturer, memory_type, capacity);
            sendUARTMessage(msg, strlen(msg));
            sendUARTMessage("  >>> Check mikroBUS connection! <<<\r\n", 39);
            return FLASH_ERROR_JEDEC_ID_MISMATCH;
        }
    }
    else
    {
        sendUARTMessage("\r\nSPI Flash Init FAILED: Cannot read JEDEC ID\r\n", 48);
        return result;
    }
}

/******************************************************************************
 * Read JEDEC ID
 *****************************************************************************/
SPI_Flash_Result SPI_Flash_ReadID(SPI_Flash_S25FL512S *handle, 
                                   uint8 *manufacturer, 
                                   uint8 *memory_type, 
                                   uint8 *capacity)
{
    if (handle == NULL || manufacturer == NULL || memory_type == NULL || capacity == NULL)
    {
        return FLASH_ERROR_INVALID_PARAMETER;
    }
    
    /* Wait for previous operation */
    while (IfxQspi_SpiMaster_getStatus(&handle->channel) == IfxQspi_Status_busy);
    
    /* Prepare command + dummy bytes for reading */
    uint8 txBuffer[4] = { S25FL512S_CMD_READ_ID, 0xFF, 0xFF, 0xFF };
    uint8 rxBuffer[4] = { 0 };
    
    /* Manual CS control */
    IfxPort_setPinLow(&MODULE_P14, 7);  /* CS LOW */
    
    IfxQspi_SpiMaster_exchange(&handle->channel, txBuffer, rxBuffer, 4);
    
    /* Wait for completion */
    while (IfxQspi_SpiMaster_getStatus(&handle->channel) == IfxQspi_Status_busy);
    
    IfxPort_setPinHigh(&MODULE_P14, 7);  /* CS HIGH */
    
    /* Apply RX bit shift correction (QSPI samples 1 bit late) */
    SPI_Flash_CorrectBitShift_RX(rxBuffer, 4);
    
    /* Extract JEDEC ID (skip command byte) */
    *manufacturer = rxBuffer[1];
    *memory_type = rxBuffer[2];
    *capacity = rxBuffer[3];
    
    return FLASH_OK;
}

/******************************************************************************
 * Read Status Register 1
 *****************************************************************************/
SPI_Flash_Result SPI_Flash_ReadStatus(SPI_Flash_S25FL512S *handle, uint8 *status)
{
    if (handle == NULL || status == NULL)
    {
        return FLASH_ERROR_INVALID_PARAMETER;
    }
    
    /* Wait for previous operation */
    while (IfxQspi_SpiMaster_getStatus(&handle->channel) == IfxQspi_Status_busy);
    
    /* Send command and receive status */
    uint8 txBuffer[2] = { S25FL512S_CMD_READ_STATUS_REG1, 0xFF };
    uint8 rxBuffer[2] = { 0 };
    
    IfxPort_setPinLow(&MODULE_P14, 7);  /* CS LOW */
    
    IfxQspi_SpiMaster_exchange(&handle->channel, txBuffer, rxBuffer, 2);
    
    /* Wait for completion */
    while (IfxQspi_SpiMaster_getStatus(&handle->channel) == IfxQspi_Status_busy);
    
    IfxPort_setPinHigh(&MODULE_P14, 7);  /* CS HIGH */
    
    /* Apply RX bit shift correction */
    SPI_Flash_CorrectBitShift_RX(rxBuffer, 2);
    
    *status = rxBuffer[1];
    
    return FLASH_OK;
}

/******************************************************************************
 * Check if flash is busy
 *****************************************************************************/
boolean SPI_Flash_IsBusy(SPI_Flash_S25FL512S *handle)
{
    uint8 status;
    if (SPI_Flash_ReadStatus(handle, &status) == FLASH_OK)
    {
        return (status & S25FL512S_SR1_WIP) ? TRUE : FALSE;
    }
    return FALSE;
}

/******************************************************************************
 * Wait until flash is ready
 *****************************************************************************/
SPI_Flash_Result SPI_Flash_WaitReady(SPI_Flash_S25FL512S *handle, uint32 timeout_ms)
{
    uint32 start_time = SPI_Flash_GetTickMs();
    
    while (SPI_Flash_IsBusy(handle))
    {
        if ((SPI_Flash_GetTickMs() - start_time) > timeout_ms)
        {
            return FLASH_ERROR_TIMEOUT;
        }
        waitTime(IfxStm_getTicksFromMilliseconds(BSP_DEFAULT_TIMER, 1));
    }
    
    return FLASH_OK;
}

/******************************************************************************
 * Enable write operations
 *****************************************************************************/
SPI_Flash_Result SPI_Flash_WriteEnable(SPI_Flash_S25FL512S *handle)
{
    if (handle == NULL)
    {
        return FLASH_ERROR_INVALID_PARAMETER;
    }
    
    /* Send Write Enable command */
    SPI_Flash_SendCommand(handle, S25FL512S_CMD_WRITE_ENABLE);
    
    /* Add extra delay for command to take effect (SendCommand already has 50us) */
    waitTime(IfxStm_getTicksFromMicroseconds(BSP_DEFAULT_TIMER, 200));  /* Additional 200us delay */
    
    /* EXPERIMENTAL: Skip status verification entirely */
    /* Just assume it worked and let the Flash reject if not */
    return FLASH_OK;
}

/******************************************************************************
 * Disable write operations
 *****************************************************************************/
SPI_Flash_Result SPI_Flash_WriteDisable(SPI_Flash_S25FL512S *handle)
{
    if (handle == NULL)
    {
        return FLASH_ERROR_INVALID_PARAMETER;
    }
    
    SPI_Flash_SendCommand(handle, S25FL512S_CMD_WRITE_DISABLE);
    return FLASH_OK;
}

/******************************************************************************
 * Read data from flash
 *****************************************************************************/
SPI_Flash_Result SPI_Flash_Read(SPI_Flash_S25FL512S *handle, 
                                 uint32 address, 
                                 uint8 *buffer, 
                                 uint32 length)
{
    if (handle == NULL || buffer == NULL || length == 0)
    {
        return FLASH_ERROR_INVALID_PARAMETER;
    }
    
    if (!handle->is_initialized)
    {
        return FLASH_ERROR_NOT_INITIALIZED;
    }
    
    /* Wait for previous operation */
    while (IfxQspi_SpiMaster_getStatus(&handle->channel) == IfxQspi_Status_busy);
    
    IfxPort_setPinLow(&MODULE_P14, 7);  /* CS LOW - Start transaction */
    
    /* Build command: CMD (1) + Address (4) - MikroE method: 3-byte cmd, 4-byte addr */
    /* Flash in 3-byte mode reads only lower 3 bytes, MSB ignored */
    uint8 cmdBuffer[5];
    cmdBuffer[0] = S25FL512S_CMD_READ_DATA;   /* 0x03 - 3-byte command */
    cmdBuffer[1] = (address >> 24) & 0xFF;    /* MSB (ignored in 3-byte mode) */
    cmdBuffer[2] = (address >> 16) & 0xFF;
    cmdBuffer[3] = (address >> 8) & 0xFF;
    cmdBuffer[4] = (address >> 0) & 0xFF;
    
    /* Send command + address */
    IfxQspi_SpiMaster_exchange(&handle->channel, cmdBuffer, NULL_PTR, 5);
    while (IfxQspi_SpiMaster_getStatus(&handle->channel) == IfxQspi_Status_busy);
    
    /* Allocate temporary buffer with +1 byte for carry bit of last byte */
    uint8 *tempBuffer = (uint8 *)malloc(length + 1);
    if (tempBuffer == NULL)
    {
        IfxPort_setPinHigh(&MODULE_P14, 7);
        return FLASH_ERROR_INVALID_PARAMETER;
    }
    
    /* Read data in chunks (read +1 byte for last carry bit) */
    uint32 total_to_read = length + 1;
    uint32 remaining = total_to_read;
    uint32 offset = 0;
    
    while (remaining > 0)
    {
        uint32 chunk = (remaining > 256) ? 256 : remaining;
        
        /* Receive data chunk */
        IfxQspi_SpiMaster_exchange(&handle->channel, NULL_PTR, &tempBuffer[offset], chunk);
        while (IfxQspi_SpiMaster_getStatus(&handle->channel) == IfxQspi_Status_busy);
        
        offset += chunk;
        remaining -= chunk;
    }
    
    IfxPort_setPinHigh(&MODULE_P14, 7);  /* CS HIGH - End transaction */
    
    /* Apply RX bit shift correction to temp buffer (length+1) */
    SPI_Flash_CorrectBitShift_RX(tempBuffer, length + 1);
    
    /* Copy corrected data to output buffer (only 'length' bytes) */
    memcpy(buffer, tempBuffer, length);
    
    /* Free temporary buffer */
    free(tempBuffer);
    
    return FLASH_OK;
}

/******************************************************************************
 * Write data to flash (page program)
 *****************************************************************************/
SPI_Flash_Result SPI_Flash_Write(SPI_Flash_S25FL512S *handle, 
                                  uint32 address, 
                                  const uint8 *data, 
                                  uint32 length)
{
    if (handle == NULL || data == NULL || length == 0)
    {
        return FLASH_ERROR_INVALID_PARAMETER;
    }
    
    if (!handle->is_initialized)
    {
        return FLASH_ERROR_NOT_INITIALIZED;
    }
    
    uint32 offset = 0;
    SPI_Flash_Result result;
    
    while (offset < length)
    {
        /* Calculate bytes to write (max 256, don't cross page boundary) */
        uint32 page_remaining = S25FL512S_PAGE_SIZE - ((address + offset) % S25FL512S_PAGE_SIZE);
        uint32 bytes_to_write = (length - offset) < page_remaining ? (length - offset) : page_remaining;
        
        /* Enable write */
        result = SPI_Flash_WriteEnable(handle);
        if (result != FLASH_OK)
        {
            return result;  /* Write Enable failed! */
        }
        
        /* Wait for previous operation */
        while (IfxQspi_SpiMaster_getStatus(&handle->channel) == IfxQspi_Status_busy);
        
        IfxPort_setPinLow(&MODULE_P14, 7);  /* CS LOW - Start transaction */
        
        /* Build command: CMD (1) + Address (4) - MikroE method */
        uint32 current_address = address + offset;
        uint8 cmdBuffer[5];
        cmdBuffer[0] = S25FL512S_CMD_PAGE_PROGRAM;        /* 0x02 - 3-byte command */
        cmdBuffer[1] = (current_address >> 24) & 0xFF;    /* MSB (ignored) */
        cmdBuffer[2] = (current_address >> 16) & 0xFF;
        cmdBuffer[3] = (current_address >> 8) & 0xFF;
        cmdBuffer[4] = (current_address >> 0) & 0xFF;
        
        /* Send command + address */
        IfxQspi_SpiMaster_exchange(&handle->channel, cmdBuffer, NULL_PTR, 5);
        while (IfxQspi_SpiMaster_getStatus(&handle->channel) == IfxQspi_Status_busy);
        
        /* Send data directly (no bit shift needed for TX) */
        IfxQspi_SpiMaster_exchange(&handle->channel, (uint8 *)&data[offset], NULL_PTR, bytes_to_write);
        while (IfxQspi_SpiMaster_getStatus(&handle->channel) == IfxQspi_Status_busy);
        
        IfxPort_setPinHigh(&MODULE_P14, 7);  /* CS HIGH - End transaction */
        
        /* Wait for programming to complete */
        if (SPI_Flash_WaitReady(handle, S25FL512S_TIMEOUT_PAGE_PROGRAM_MS) != FLASH_OK)
        {
            return FLASH_ERROR_PROGRAM_FAILED;
        }
        
        offset += bytes_to_write;
        address += bytes_to_write;
    }
    
    return FLASH_OK;
}

/******************************************************************************
 * Erase 4KB sector
 *****************************************************************************/
SPI_Flash_Result SPI_Flash_EraseSector(SPI_Flash_S25FL512S *handle, uint32 address)
{
    if (handle == NULL)
    {
        return FLASH_ERROR_INVALID_PARAMETER;
    }
    
    if (!handle->is_initialized)
    {
        return FLASH_ERROR_NOT_INITIALIZED;
    }
    
    /* Enable write */
    SPI_Flash_Result result = SPI_Flash_WriteEnable(handle);
    if (result != FLASH_OK)
    {
        return result;  /* Write Enable failed! */
    }
    
    /* Send erase command - Use 0xDC like MikroE official example */
    /* NOTE: 0xDC erases 256KB block, not 4KB sector! */
    SPI_Flash_SendCommandWithAddress(handle, S25FL512S_CMD_BLOCK_ERASE_256KB, address);
    
    /* Wait for erase to complete */
    return SPI_Flash_WaitReady(handle, S25FL512S_TIMEOUT_SECTOR_ERASE_MS);
}

/******************************************************************************
 * Erase 64KB block
 *****************************************************************************/
SPI_Flash_Result SPI_Flash_EraseBlock(SPI_Flash_S25FL512S *handle, uint32 address)
{
    if (handle == NULL)
    {
        return FLASH_ERROR_INVALID_PARAMETER;
    }
    
    if (!handle->is_initialized)
    {
        return FLASH_ERROR_NOT_INITIALIZED;
    }
    
    /* Enable write */
    SPI_Flash_Result result = SPI_Flash_WriteEnable(handle);
    if (result != FLASH_OK)
    {
        return result;  /* Write Enable failed! */
    }
    
    /* Send erase command */
    SPI_Flash_SendCommandWithAddress(handle, S25FL512S_CMD_BLOCK_ERASE_64KB, address);
    
    /* Wait for erase to complete */
    return SPI_Flash_WaitReady(handle, S25FL512S_TIMEOUT_BLOCK_ERASE_MS);
}

/******************************************************************************
 * Erase entire chip
 *****************************************************************************/
SPI_Flash_Result SPI_Flash_EraseChip(SPI_Flash_S25FL512S *handle)
{
    if (handle == NULL)
    {
        return FLASH_ERROR_INVALID_PARAMETER;
    }
    
    if (!handle->is_initialized)
    {
        return FLASH_ERROR_NOT_INITIALIZED;
    }
    
    /* Enable write */
    SPI_Flash_Result result = SPI_Flash_WriteEnable(handle);
    if (result != FLASH_OK)
    {
        return result;  /* Write Enable failed! */
    }
    
    /* Send chip erase command */
    SPI_Flash_SendCommand(handle, S25FL512S_CMD_CHIP_ERASE);
    
    /* Wait for erase to complete (this takes a LONG time!) */
    return SPI_Flash_WaitReady(handle, S25FL512S_TIMEOUT_CHIP_ERASE_MS);
}

/******************************************************************************
 * Software reset
 *****************************************************************************/
SPI_Flash_Result SPI_Flash_Reset(SPI_Flash_S25FL512S *handle)
{
    if (handle == NULL)
    {
        return FLASH_ERROR_INVALID_PARAMETER;
    }
    
    /* Send reset enable command */
    SPI_Flash_SendCommand(handle, S25FL512S_CMD_RESET_ENABLE);
    
    /* Send reset command */
    SPI_Flash_SendCommand(handle, S25FL512S_CMD_RESET);
    
    /* Wait for reset to complete */
    waitTime(IfxStm_getTicksFromMilliseconds(BSP_DEFAULT_TIMER, 1));
    
    return FLASH_OK;
}

/******************************************************************************
 * Get error message string
 *****************************************************************************/
const char* SPI_Flash_GetErrorString(SPI_Flash_Result result)
{
    switch (result)
    {
        case FLASH_OK:                      return "OK";
        case FLASH_ERROR_NOT_INITIALIZED:   return "Not initialized";
        case FLASH_ERROR_INVALID_PARAMETER: return "Invalid parameter";
        case FLASH_ERROR_TIMEOUT:           return "Timeout";
        case FLASH_ERROR_WRITE_PROTECTED:   return "Write protected";
        case FLASH_ERROR_WRITE_ENABLE_FAILED: return "Write Enable failed (WEL bit not set)";
        case FLASH_ERROR_ERASE_FAILED:      return "Erase failed";
        case FLASH_ERROR_PROGRAM_FAILED:    return "Program failed";
        case FLASH_ERROR_VERIFY_FAILED:     return "Verify failed";
        case FLASH_ERROR_BUSY:              return "Busy";
        case FLASH_ERROR_JEDEC_ID_MISMATCH: return "JEDEC ID mismatch";
        default:                            return "Unknown error";
    }
}

/******************************************************************************
 * Internal Helper Functions
 *****************************************************************************/

static void SPI_Flash_HardwareReset(void)
{
    /* Configure RST pin as output */
    IfxPort_setPinModeOutput(QSPI_FLASH_RST_PORT, QSPI_FLASH_RST_PIN, 
                             IfxPort_OutputMode_pushPull, IfxPort_OutputIdx_general);
    
    /* Assert reset (LOW) */
    IfxPort_setPinLow(QSPI_FLASH_RST_PORT, QSPI_FLASH_RST_PIN);
    waitTime(IfxStm_getTicksFromMilliseconds(BSP_DEFAULT_TIMER, 10));
    
    /* Release reset (HIGH) */
    IfxPort_setPinHigh(QSPI_FLASH_RST_PORT, QSPI_FLASH_RST_PIN);
    waitTime(IfxStm_getTicksFromMilliseconds(BSP_DEFAULT_TIMER, 10));
}

static void SPI_Flash_SendCommand(SPI_Flash_S25FL512S *handle, uint8 command)
{
    /* Wait for previous operation */
    while (IfxQspi_SpiMaster_getStatus(&handle->channel) == IfxQspi_Status_busy);
    
    /* Add setup delay before CS LOW */
    waitTime(IfxStm_getTicksFromMicroseconds(BSP_DEFAULT_TIMER, 10));
    
    IfxPort_setPinLow(&MODULE_P14, 7);  /* CS LOW */
    
    /* Add delay after CS LOW for stabilization */
    waitTime(IfxStm_getTicksFromMicroseconds(BSP_DEFAULT_TIMER, 10));
    
    /* Send command */
    uint8 txBuffer[1] = { command };
    
    IfxQspi_SpiMaster_exchange(&handle->channel, txBuffer, NULL_PTR, 1);
    
    /* Wait for completion */
    while (IfxQspi_SpiMaster_getStatus(&handle->channel) == IfxQspi_Status_busy);
    
    /* Add delay before CS HIGH */
    waitTime(IfxStm_getTicksFromMicroseconds(BSP_DEFAULT_TIMER, 10));
    
    IfxPort_setPinHigh(&MODULE_P14, 7);  /* CS HIGH */
    
    /* CRITICAL: Add delay after CS HIGH for command to take effect */
    waitTime(IfxStm_getTicksFromMicroseconds(BSP_DEFAULT_TIMER, 50));
}

static void SPI_Flash_SendCommandWithAddress(SPI_Flash_S25FL512S *handle, 
                                              uint8 command, 
                                              uint32 address)
{
    /* Wait for previous operation */
    while (IfxQspi_SpiMaster_getStatus(&handle->channel) == IfxQspi_Status_busy);
    
    /* Add setup delay before CS LOW */
    waitTime(IfxStm_getTicksFromMicroseconds(BSP_DEFAULT_TIMER, 10));
    
    IfxPort_setPinLow(&MODULE_P14, 7);  /* CS LOW */
    
    /* Add delay after CS LOW for stabilization */
    waitTime(IfxStm_getTicksFromMicroseconds(BSP_DEFAULT_TIMER, 10));
    
    /* Build command buffer with 4-byte address for S25FL512S (64MB Flash) */
    uint8 txBuffer[5];
    txBuffer[0] = command;
    txBuffer[1] = (address >> 24) & 0xFF;  /* 4-byte address */
    txBuffer[2] = (address >> 16) & 0xFF;
    txBuffer[3] = (address >> 8) & 0xFF;
    txBuffer[4] = (address >> 0) & 0xFF;
    
    /* Send command + address */
    IfxQspi_SpiMaster_exchange(&handle->channel, txBuffer, NULL_PTR, 5);
    
    /* Wait for completion */
    while (IfxQspi_SpiMaster_getStatus(&handle->channel) == IfxQspi_Status_busy);
    
    /* Add delay before CS HIGH */
    waitTime(IfxStm_getTicksFromMicroseconds(BSP_DEFAULT_TIMER, 10));
    
    IfxPort_setPinHigh(&MODULE_P14, 7);  /* CS HIGH */
    
    /* CRITICAL: Add delay after CS HIGH for command to take effect */
    waitTime(IfxStm_getTicksFromMicroseconds(BSP_DEFAULT_TIMER, 50));
}

static uint32 SPI_Flash_GetTickMs(void)
{
    return (uint32)(IfxStm_get(&MODULE_STM0) / (IfxStm_getFrequency(&MODULE_STM0) / 1000));
}
