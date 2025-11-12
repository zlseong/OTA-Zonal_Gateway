/**
 * @file Flash4_Driver.h
 * @brief MIKROE-3191 Flash 4 Click Driver for TC375 Lite Kit
 */

#ifndef FLASH4_DRIVER_H_
#define FLASH4_DRIVER_H_

#include "Ifx_Types.h"
#include "IfxQspi_SpiMaster.h"
#include "IfxPort.h"
#include "Flash4_Config.h"

/* Flash Commands */
#define FLASH4_CMD_READ_IDENTIFICATION           0x9F
#define FLASH4_CMD_READ_STATUS_REG_1             0x05
#define FLASH4_CMD_WRITE_ENABLE_WREN             0x06
#define FLASH4_CMD_WRITE_DISABLE_WRDI            0x04
#define FLASH4_CMD_READ_FLASH                    0x03
#define FLASH4_CMD_PAGE_PROGRAM                  0x02
#define FLASH4_CMD_SECTOR_ERASE                  0xD8
#define FLASH4_CMD_RESET_ENABLE                  0x66
#define FLASH4_CMD_RESET                         0x99

/* Flash Device IDs (S25FL512S datasheet) */
#define FLASH4_MANUFACTURER_ID                   0x01
#define FLASH4_DEVICE_ID_MSB                     0x02
#define FLASH4_DEVICE_ID_LSB                     0x20

/* Configuration */
#define FLASH4_MAX_PAGE_SIZE                     512

/* Return Values */
#define FLASH4_OK                                0
#define FLASH4_TIMEOUT                           3

/* Function Prototypes */
void Flash4_Init(void);
void Flash4_WriteCommand(uint8 cmd);
void Flash4_ReadManufacturerId(uint8 *deviceId);
void Flash4_ReadFlash4(uint32 address, uint8 *outData, uint16 nData);
void Flash4_PageProgram(uint32 address, const uint8 *data, uint16 length);
void Flash4_SectorErase(uint32 address);
void Flash4_WriteEnable(void);
boolean Flash4_CheckWIP(void);
uint8 Flash4_ReadStatusReg(void);
uint8 Flash4_WaitReady(uint32 timeoutMs);

#endif /* FLASH4_DRIVER_H_ */

