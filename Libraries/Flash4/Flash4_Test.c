/**********************************************************************************************************************
 * \file Flash4_Test.c
 * \copyright Copyright (C) Infineon Technologies AG 2019
 * 
 * Flash4 Test Suite Implementation
 *********************************************************************************************************************/

#include "Flash4_Test.h"
#include "Flash4_Driver.h"
#include "UART_Logging.h"
#include <string.h>
#include <stdio.h>

void Test_Flash4(void)
{
    char msg[128];
    uint8 testData[256];
    uint8 readData[256];
    uint16 i;
    uint32 testAddr = 0x000000;
    
    sendUARTMessage("\r\n========================================\r\n", 43);
    sendUARTMessage("Flash4 Test: Start\r\n", 20);
    sendUARTMessage("========================================\r\n", 41);
    
    sendUARTMessage("Test 1: Read Manufacturer ID...\r\n", 34);
    uint8 deviceId[3];
    Flash4_ReadManufacturerId(deviceId);
    sprintf(msg, "  Manufacturer ID: 0x%02X\r\n", deviceId[0]);
    sendUARTMessage(msg, strlen(msg));
    sprintf(msg, "  Device ID: 0x%02X 0x%02X\r\n", deviceId[1], deviceId[2]);
    sendUARTMessage(msg, strlen(msg));
    
    sendUARTMessage("\r\nTest 2: Sector Erase (256KB at 0x000000)...\r\n", 47);
    Flash4_SectorErase(testAddr);
    sendUARTMessage("  Waiting for erase to complete...\r\n", 37);
    if (Flash4_WaitReady(3000) == FLASH4_OK)
    {
        sendUARTMessage("  Erase complete!\r\n", 19);
    }
    else
    {
        sendUARTMessage("  Erase TIMEOUT!\r\n", 18);
    }
    
    sendUARTMessage("\r\nTest 3: Page Program (256 bytes)...\r\n", 39);
    for (i = 0; i < 256; i++)
    {
        testData[i] = (uint8)(i & 0xFF);
    }
    Flash4_PageProgram(testAddr, testData, 256);
    sendUARTMessage("  Write complete!\r\n", 19);
    
    sendUARTMessage("\r\nTest 4: Read Flash and Verify...\r\n", 36);
    Flash4_ReadFlash4(testAddr, readData, 256);
    
    boolean verifyOK = TRUE;
    for (i = 0; i < 256; i++)
    {
        if (readData[i] != testData[i])
        {
            verifyOK = FALSE;
            sprintf(msg, "  MISMATCH at offset %d: wrote 0x%02X, read 0x%02X\r\n", 
                    i, testData[i], readData[i]);
            sendUARTMessage(msg, strlen(msg));
            break;
        }
    }
    
    if (verifyOK)
    {
        sendUARTMessage("  Verify PASSED! All 256 bytes match.\r\n", 40);
    }
    else
    {
        sendUARTMessage("  Verify FAILED!\r\n", 18);
    }
    
    sendUARTMessage("\r\nTest 5: Read Status Register...\r\n", 35);
    uint8 status = Flash4_ReadStatusReg();
    sprintf(msg, "  Status Register: 0x%02X\r\n", status);
    sendUARTMessage(msg, strlen(msg));
    sprintf(msg, "    WIP (bit 0): %d\r\n", (status & 0x01) ? 1 : 0);
    sendUARTMessage(msg, strlen(msg));
    sprintf(msg, "    WEL (bit 1): %d\r\n", (status & 0x02) ? 1 : 0);
    sendUARTMessage(msg, strlen(msg));
    
    sendUARTMessage("\r\n========================================\r\n", 43);
    sendUARTMessage("Flash4 Test: Complete\r\n", 23);
    sendUARTMessage("========================================\r\n\r\n", 43);
}

