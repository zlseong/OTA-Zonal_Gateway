/**********************************************************************************************************************
 * \file Cpu0_Main.c
 * \copyright Copyright (C) Infineon Technologies AG 2019
 * 
 * Zonal Gateway - Ethernet + lwIP Integration
 * Target: TC375 Lite Kit
 * IP: 192.168.1.10 (Static)
 *********************************************************************************************************************/
#include "Ifx_Types.h"
#include "IfxCpu.h"
#include "IfxScuWdt.h"
#include "IfxStm.h"
#include "IfxGeth_Eth.h"
#include "Ifx_Lwip.h"
#include "Configuration.h"
#include "ConfigurationIsr.h"
#include "UART_Logging.h"
#include "lwip/tcp.h"
#include "lwip/udp.h"
#include "lwip/pbuf.h"
#include "Libraries/DoIP/doip_client.h"
#include "Libraries/DoIP/uds_handler.h"
#include "Libraries/DoIP/uds_download.h"
#include "Libraries/DoIP/software_package.h"
#include "Libraries/SPI_Flash/spi_flash_s25fl512s.h"
/* Bit-bang test removed - using official iLLD example method */
#include <string.h>
#include <stdio.h>
#include <stdlib.h>

IFX_ALIGN(4) IfxCpu_syncEvent g_cpuSyncEvent = 0;

/* TCP Echo Server (포트 8765) */
#define TCP_ECHO_PORT  8765

/* UDP Echo Server (포트 13400) */
#define UDP_DOIP_PORT  13400

/* VCI Magic Number */
#define VCI_MAGIC  0x56434921  /* "VCI!" */

static struct tcp_pcb *g_tcp_server_pcb = NULL;
static struct udp_pcb *g_udp_server_pcb = NULL;

/* Global VCI Database (accessed by UDS Handler) */
DoIP_VCI_Info g_vci_database[MAX_ZONE_ECUS + 1];  /* +1 for ZG itself */
uint8 g_zone_ecu_count = 0;  /* Count of Zone ECUs (not including ZG) */
boolean g_vci_collection_complete = FALSE;
DoIP_VCI_Info g_zgw_vci;  /* ZG's own VCI */

/* Global Health Status Database (accessed by UDS Handler) */
DoIP_HealthStatus_Info g_health_data[MAX_ZONE_ECUS + 1];  /* +1 for ZG itself */

/* Global SPI Flash Handle */
SPI_Flash_S25FL512S g_spi_flash;

/* ============================================
 * SPI Flash Test Function
 * ============================================ */
void Test_SPI_Flash(void)
{
    char msg[128];
    SPI_Flash_Result result;
    
    sendUARTMessage("\r\n===========================================\r\n", 46);
    sendUARTMessage("[SPI Flash] Testing S25FL512S (64MB)\r\n", 39);
    sendUARTMessage("===========================================\r\n", 44);
    
    /* Test 1: Read JEDEC ID */
    sendUARTMessage("[Test 1] Reading JEDEC ID...\r\n", 30);
    sprintf(msg, "  Manufacturer: 0x%02X (Expected: 0x01)\r\n", g_spi_flash.manufacturer_id);
    sendUARTMessage(msg, strlen(msg));
    sprintf(msg, "  Memory Type:  0x%02X (Expected: 0x02)\r\n", g_spi_flash.memory_type);
    sendUARTMessage(msg, strlen(msg));
    sprintf(msg, "  Capacity:     0x%02X (Expected: 0x20 = 512Mbit)\r\n", g_spi_flash.capacity);
    sendUARTMessage(msg, strlen(msg));
    
    /* Test 2: Read Status Registers */
    sendUARTMessage("[Test 2] Reading Status Registers...\r\n", 39);
    uint8 status = 0;
    result = SPI_Flash_ReadStatus(&g_spi_flash, &status);
    if (result == FLASH_OK)
    {
        sprintf(msg, "  Status Reg 1: 0x%02X (WIP=%d, WEL=%d, BP=%d)\r\n", 
                status, 
                (status & S25FL512S_SR1_WIP) ? 1 : 0,
                (status & S25FL512S_SR1_WEL) ? 1 : 0,
                (status >> 2) & 0x1F);  /* BP0-BP4 bits */
        sendUARTMessage(msg, strlen(msg));
        
        /* Read Status Register 2 */
        uint8 txBuf[2] = { 0x07, 0xFF };  /* Read Status Reg 2 command */
        uint8 rxBuf[2] = { 0 };
        IfxPort_setPinLow(&MODULE_P14, 7);
        IfxQspi_SpiMaster_exchange(&g_spi_flash.channel, txBuf, rxBuf, 2);
        while (IfxQspi_SpiMaster_getStatus(&g_spi_flash.channel) == IfxQspi_Status_busy);
        IfxPort_setPinHigh(&MODULE_P14, 7);
        
        /* Apply RX shift correction to both bytes */
        uint32 k;
        for (k = 0; k < 2; k++) {
            uint8 corrected = rxBuf[k] >> 1;
            if (k < 1 && (rxBuf[k+1] & 0x01)) {
                corrected |= 0x80;
            }
            rxBuf[k] = corrected;
        }
        
        sprintf(msg, "  Status Reg 2: 0x%02X (raw bytes: %02X %02X)\r\n", 
                rxBuf[1], rxBuf[0], rxBuf[1]);
        sendUARTMessage(msg, strlen(msg));
        
        /* Read Configuration Register (0x35) */
        uint8 txCfg[5] = { 0x35, 0xFF, 0xFF, 0xFF, 0xFF };  /* Read Config Reg */
        uint8 rxCfg[5] = { 0 };
        IfxPort_setPinLow(&MODULE_P14, 7);
        IfxQspi_SpiMaster_exchange(&g_spi_flash.channel, txCfg, rxCfg, 5);
        while (IfxQspi_SpiMaster_getStatus(&g_spi_flash.channel) == IfxQspi_Status_busy);
        IfxPort_setPinHigh(&MODULE_P14, 7);
        
        /* Apply RX shift correction */
        for (k = 0; k < 5; k++) {
            uint8 corrected = rxCfg[k] >> 1;
            if (k < 4 && (rxCfg[k+1] & 0x01)) {
                corrected |= 0x80;
            }
            rxCfg[k] = corrected;
        }
        
        sprintf(msg, "  Config Reg: %02X %02X %02X %02X (TBPROT=%d, BPNV=%d, TBPARM=%d, QUAD=%d)\r\n",
                rxCfg[1], rxCfg[2], rxCfg[3], rxCfg[4],
                (rxCfg[1] >> 5) & 0x01,  /* TBPROT */
                (rxCfg[1] >> 3) & 0x01,  /* BPNV */
                (rxCfg[1] >> 2) & 0x01,  /* TBPARM */
                (rxCfg[2] >> 1) & 0x01); /* QUAD */
        sendUARTMessage(msg, strlen(msg));
        
        /* Check if hardware WP# is active */
        sendUARTMessage("\r\n  >>> CRITICAL HARDWARE CHECK <<<\r\n", 37);
        sendUARTMessage("  If all software attempts fail, check:\r\n", 42);
        sendUARTMessage("  1. MIKROE-3191 WP# pin (Pin 5 on Flash chip)\r\n", 49);
        sendUARTMessage("  2. Must be HIGH (VCC) for Write/Erase\r\n", 42);
        sendUARTMessage("  3. If tied to GND, Flash is PERMANENTLY Write Protected!\r\n", 61);
        sendUARTMessage("  =====================================\r\n\r\n", 43);
    }
    else
    {
        sendUARTMessage("  ERROR: Failed to read status\r\n", 33);
        return;
    }
    
    /* Test 3: Erase Sector at 0x001000 (4KB) */
    sendUARTMessage("[Test 3] Erasing 4KB sector at 0x001000...\r\n", 46);
    result = SPI_Flash_EraseSector(&g_spi_flash, 0x001000);
    if (result == FLASH_OK)
    {
        sendUARTMessage("  OK: Sector erased\r\n", 20);
        
        /* Verify erase: read back and check if all 0xFF */
        uint8 erase_check[16];
        result = SPI_Flash_Read(&g_spi_flash, 0x001000, erase_check, 16);
        if (result == FLASH_OK)
        {
            uint32 erase_ok = 1;
            uint32 k;
            for (k = 0; k < 16; k++)
            {
                if (erase_check[k] != 0xFF)
                {
                    erase_ok = 0;
                    break;
                }
            }
            if (erase_ok)
            {
                sendUARTMessage("  Erase verified: all 0xFF\r\n", 28);
            }
            else
            {
                sendUARTMessage("  WARNING: Erase incomplete! First 16: ", 40);
                for (k = 0; k < 16; k++)
                {
                    sprintf(msg, "%02X ", erase_check[k]);
                    sendUARTMessage(msg, strlen(msg));
                }
                sendUARTMessage("\r\n", 2);
            }
        }
    }
    else
    {
        sprintf(msg, "  ERROR: %s\r\n", SPI_Flash_GetErrorString(result));
        sendUARTMessage(msg, strlen(msg));
        return;
    }
    
    /* Test 4: Write Data */
    sendUARTMessage("[Test 4] Writing 64 bytes to 0x001000...\r\n", 43);
    uint8 write_data[64];
    for (uint32 i = 0; i < 64; i++)
    {
        write_data[i] = (uint8)(i + 0xA0);  /* Pattern: 0xA0, 0xA1, ... 0xDF */
    }
    
    /* Show first 16 bytes to be written */
    sendUARTMessage("  Data to write (first 16): ", 29);
    {
        uint32 k;
        for (k = 0; k < 16; k++)
        {
            sprintf(msg, "%02X ", write_data[k]);
            sendUARTMessage(msg, strlen(msg));
        }
        sendUARTMessage("\r\n", 2);
    }
    
    result = SPI_Flash_Write(&g_spi_flash, 0x001000, write_data, 64);
    if (result == FLASH_OK)
    {
        sendUARTMessage("  OK: Data written\r\n", 19);
        
        /* Check status after write */
        uint8 status_after_write = 0;
        if (SPI_Flash_ReadStatus(&g_spi_flash, &status_after_write) == FLASH_OK)
        {
            sprintf(msg, "  Status after write: 0x%02X\r\n", status_after_write);
            sendUARTMessage(msg, strlen(msg));
        }
    }
    else
    {
        sprintf(msg, "  ERROR: %s\r\n", SPI_Flash_GetErrorString(result));
        sendUARTMessage(msg, strlen(msg));
        return;
    }
    
    /* Test 5: Read Data Back */
    sendUARTMessage("[Test 5] Reading 64 bytes from 0x001000...\r\n", 46);
    uint8 read_data[64];
    memset(read_data, 0, 64);
    
    result = SPI_Flash_Read(&g_spi_flash, 0x001000, read_data, 64);
    if (result == FLASH_OK)
    {
        sendUARTMessage("  OK: Data read\r\n", 16);
    }
    else
    {
        sprintf(msg, "  ERROR: %s\r\n", SPI_Flash_GetErrorString(result));
        sendUARTMessage(msg, strlen(msg));
        return;
    }
    
    /* Test 6: Verify Data */
    sendUARTMessage("[Test 6] Verifying data...\r\n", 28);
    uint32 errors = 0;
    uint32 first_error_index = 0;
    uint8 first_error_expected = 0;
    uint8 first_error_actual = 0;
    
    for (uint32 i = 0; i < 64; i++)
    {
        if (read_data[i] != write_data[i])
        {
            if (errors == 0)
            {
                first_error_index = i;
                first_error_expected = write_data[i];
                first_error_actual = read_data[i];
            }
            errors++;
        }
    }
    
    if (errors == 0)
    {
        sendUARTMessage("  OK: Data verified (100% match)\r\n", 34);
    }
    else
    {
        sprintf(msg, "  ERROR: %u byte(s) mismatch\r\n", errors);
        sendUARTMessage(msg, strlen(msg));
        sprintf(msg, "  First error at byte %u: Expected 0x%02X, Got 0x%02X\r\n", 
                first_error_index, first_error_expected, first_error_actual);
        sendUARTMessage(msg, strlen(msg));
    }
    
    /* Display first 16 bytes */
    sendUARTMessage("  First 16 bytes: ", 18);
    for (uint32 i = 0; i < 16; i++)
    {
        sprintf(msg, "%02X ", read_data[i]);
        sendUARTMessage(msg, strlen(msg));
    }
    sendUARTMessage("\r\n", 2);
    
    /* Test 7: Test Large Write (1KB) */
    sendUARTMessage("[Test 7] Testing 1KB write at 0x002000...\r\n", 45);
    uint8 *large_data = (uint8 *)malloc(1024);
    if (large_data != NULL)
    {
        for (uint32 i = 0; i < 1024; i++)
        {
            large_data[i] = (uint8)(i & 0xFF);
        }
        
        result = SPI_Flash_EraseSector(&g_spi_flash, 0x002000);
        if (result == FLASH_OK)
        {
            result = SPI_Flash_Write(&g_spi_flash, 0x002000, large_data, 1024);
            if (result == FLASH_OK)
            {
                sendUARTMessage("  OK: 1KB written\r\n", 18);
                
                /* Read back and verify */
                uint8 *verify_data = (uint8 *)malloc(1024);
                if (verify_data != NULL)
                {
                    result = SPI_Flash_Read(&g_spi_flash, 0x002000, verify_data, 1024);
                    if (result == FLASH_OK)
                    {
                        errors = 0;
                        uint32 first_1kb_error_idx = 0;
                        uint8 first_1kb_error_exp = 0;
                        uint8 first_1kb_error_act = 0;
                        
                        for (uint32 i = 0; i < 1024; i++)
                        {
                            if (verify_data[i] != large_data[i])
                            {
                                if (errors == 0)
                                {
                                    first_1kb_error_idx = i;
                                    first_1kb_error_exp = large_data[i];
                                    first_1kb_error_act = verify_data[i];
                                }
                                errors++;
                            }
                        }
                        
                        if (errors == 0)
                        {
                            sendUARTMessage("  OK: 1KB verified (100% match)\r\n", 33);
                        }
                        else
                        {
                            sprintf(msg, "  ERROR: %u byte(s) mismatch in 1KB test\r\n", errors);
                            sendUARTMessage(msg, strlen(msg));
                            sprintf(msg, "  First 1KB error at byte %u: Expected 0x%02X, Got 0x%02X\r\n", 
                                    first_1kb_error_idx, first_1kb_error_exp, first_1kb_error_act);
                            sendUARTMessage(msg, strlen(msg));
                            
                            /* Show surrounding bytes for context */
                            if (first_1kb_error_idx >= 2 && first_1kb_error_idx < 1022)
                            {
                                sendUARTMessage("  Context bytes [", 17);
                                for (uint32 j = first_1kb_error_idx - 2; j <= first_1kb_error_idx + 2; j++)
                                {
                                    sprintf(msg, "%u", j);
                                    sendUARTMessage(msg, strlen(msg));
                                    sendUARTMessage("]: Exp=0x", 9);
                                    sprintf(msg, "%02X Got=0x%02X  ", large_data[j], verify_data[j]);
                                    sendUARTMessage(msg, strlen(msg));
                                }
                                sendUARTMessage("\r\n", 2);
                            }
                        }
                    }
                    free(verify_data);
                }
            }
            else
            {
                sprintf(msg, "  ERROR: Write failed - %s\r\n", SPI_Flash_GetErrorString(result));
                sendUARTMessage(msg, strlen(msg));
            }
        }
        free(large_data);
    }
    
    sendUARTMessage("===========================================\r\n", 44);
    sendUARTMessage("[SPI Flash] Test Complete!\r\n", 28);
    sendUARTMessage("===========================================\r\n\r\n", 46);
}

/* ============================================
 * TCP Echo Server Callbacks
 * ============================================ */

/* 데이터 수신 콜백 (Echo: 받은 데이터를 그대로 전송) */
static err_t tcp_echo_recv_callback(void *arg, struct tcp_pcb *tpcb, struct pbuf *p, err_t err)
{
    err_t ret_err;
    
    /* 연결 종료 (p == NULL) */
    if (p == NULL)
    {
        sendUARTMessage("TCP Client Disconnected\r\n", 26);
        tcp_close(tpcb);
        return ERR_OK;
    }
    
    /* 데이터 수신 확인 (TCP ACK) */
    tcp_recved(tpcb, p->tot_len);
    
    /* Echo: 받은 데이터를 그대로 전송 */
    ret_err = tcp_write(tpcb, p->payload, p->tot_len, TCP_WRITE_FLAG_COPY);
    
    if (ret_err == ERR_OK)
    {
        tcp_output(tpcb);  /* 즉시 전송 */
        
        /* UART 로그 */
        char log[64];
        sprintf(log, "TCP Echo: %d bytes\r\n", p->tot_len);
        sendUARTMessage(log, strlen(log));
    }
    
    /* pbuf 해제 */
    pbuf_free(p);
    
    return ERR_OK;
}

/* 에러 콜백 */
static void tcp_echo_error_callback(void *arg, err_t err)
{
    sendUARTMessage("TCP Error\r\n", 11);
}

/* 새로운 클라이언트 연결 수락 콜백 */
static err_t tcp_echo_accept_callback(void *arg, struct tcp_pcb *newpcb, err_t err)
{
    sendUARTMessage("TCP Client Connected\r\n", 22);
    
    /* 콜백 함수 등록 */
    tcp_recv(newpcb, tcp_echo_recv_callback);
    tcp_err(newpcb, tcp_echo_error_callback);
    
    return ERR_OK;
}

/* TCP Echo Server 초기화 */
static void tcp_echo_server_init(void)
{
    err_t err;
    
    /* 1. PCB 생성 */
    g_tcp_server_pcb = tcp_new();
    if (g_tcp_server_pcb == NULL)
    {
        sendUARTMessage("TCP PCB creation failed\r\n", 26);
        return;
    }
    
    /* 2. 포트 바인딩 (0.0.0.0:8765) */
    err = tcp_bind(g_tcp_server_pcb, IP_ADDR_ANY, TCP_ECHO_PORT);
    if (err != ERR_OK)
    {
        sendUARTMessage("TCP Bind failed\r\n", 17);
        tcp_close(g_tcp_server_pcb);
        return;
    }
    
    /* 3. Listen 상태로 전환 */
    g_tcp_server_pcb = tcp_listen(g_tcp_server_pcb);
    if (g_tcp_server_pcb == NULL)
    {
        sendUARTMessage("TCP Listen failed\r\n", 19);
        return;
    }
    
    /* 4. Accept 콜백 등록 */
    tcp_accept(g_tcp_server_pcb, tcp_echo_accept_callback);
    
    sendUARTMessage("TCP Echo Server started on port 8765\r\n", 38);
}

/* ============================================
 * UDP Echo Server Callbacks
 * ============================================ */

/* UDP 데이터 수신 콜백 (VCI 메시지 파싱) */
static void udp_echo_recv_callback(void *arg, struct udp_pcb *upcb, struct pbuf *p,
                                   const ip_addr_t *addr, u16_t port)
{
    if (p == NULL) {
        return;
    }
    
    /* Check if this is a VCI message (Magic Number + 48 bytes) */
    if (p->tot_len == (4 + 48)) {
        uint8 buffer[52];
        pbuf_copy_partial(p, buffer, 52, 0);
        
        /* Check VCI magic number */
        uint32 magic = ((uint32)buffer[0] << 24) | ((uint32)buffer[1] << 16) |
                       ((uint32)buffer[2] << 8) | buffer[3];
        
        if (magic == VCI_MAGIC && g_zone_ecu_count < MAX_ZONE_ECUS) {
            /* Parse VCI data */
            DoIP_VCI_Info *vci = &g_vci_database[g_zone_ecu_count];
            memcpy(vci->ecu_id, &buffer[4], 16);
            memcpy(vci->sw_version, &buffer[20], 8);
            memcpy(vci->hw_version, &buffer[28], 8);
            memcpy(vci->serial_num, &buffer[36], 16);
            
            g_zone_ecu_count++;
            
            /* Log received VCI */
            sendUARTMessage("[VCI] Received from ", 20);
            sendUARTMessage(vci->ecu_id, strlen(vci->ecu_id));
            sendUARTMessage(" (", 2);
            char count_str[16];
            sprintf(count_str, "%d/%d", g_zone_ecu_count, MAX_ZONE_ECUS);
            sendUARTMessage(count_str, strlen(count_str));
            sendUARTMessage(")\r\n", 3);
            
            /* Check if collection complete */
            if (g_zone_ecu_count == MAX_ZONE_ECUS && !g_vci_collection_complete) {
                sendUARTMessage("[VCI] Collection complete! Adding ZG VCI...\r\n", 46);
                
                /* Add ZG's own VCI */
                memcpy(&g_vci_database[g_zone_ecu_count], &g_zgw_vci, sizeof(DoIP_VCI_Info));
                g_vci_collection_complete = TRUE;
                
                sendUARTMessage("[VCI] Ready to send to VMG\r\n", 29);
            }
        }
    }
    
    /* pbuf 해제 */
    pbuf_free(p);
}

/* UDP Echo Server 초기화 */
static void udp_echo_server_init(void)
{
    err_t err;
    
    /* 1. PCB 생성 */
    g_udp_server_pcb = udp_new();
    if (g_udp_server_pcb == NULL) {
        sendUARTMessage("UDP PCB creation failed\r\n", 26);
        return;
    }
    
    /* 2. 포트 바인딩 (0.0.0.0:13400) */
    err = udp_bind(g_udp_server_pcb, IP_ADDR_ANY, UDP_DOIP_PORT);
    if (err != ERR_OK) {
        sendUARTMessage("UDP Bind failed\r\n", 17);
        udp_remove(g_udp_server_pcb);
        return;
    }
    
    /* 3. 수신 콜백 등록 */
    udp_recv(g_udp_server_pcb, udp_echo_recv_callback, NULL);
    
    sendUARTMessage("UDP Echo Server started on port 13400\r\n", 40);
}

/* lwIP Timer ISR (1ms period) */
IFX_INTERRUPT(updateLwIPStackISR, 0, ISR_PRIORITY_OS_TICK);
void updateLwIPStackISR(void)
{
    /* Configure STM to generate next interrupt in 1ms */
    IfxStm_increaseCompare(&MODULE_STM0, IfxStm_Comparator_0, IFX_CFG_STM_TICKS_PER_MS);
    
    /* Increase lwIP system time */
    g_TickCount_1ms++;
    
    /* Update lwIP timers for all enabled protocols (ARP, TCP, DHCP, LINK) */
    Ifx_Lwip_onTimerTick();
}

void core0_main(void)
{
    /* Enable global interrupts */
    IfxCpu_enableInterrupts();
    
    /* !!WATCHDOG0 AND SAFETY WATCHDOG ARE DISABLED HERE!!
     * Enable the watchdogs and service them periodically if it is required
     */
    IfxScuWdt_disableCpuWatchdog(IfxScuWdt_getCpuWatchdogPassword());
    IfxScuWdt_disableSafetyWatchdog(IfxScuWdt_getSafetyWatchdogPassword());
    
    /* Wait for CPU sync event */
    IfxCpu_emitEvent(&g_cpuSyncEvent);
    IfxCpu_waitEvent(&g_cpuSyncEvent, 1);
    
    /* ============================================
     * UART Initialization (Debug Logging)
     * ============================================ */
    initUART();
    sendUARTMessage("Zonal Gateway Starting...\r\n", 28);
    
    /* ============================================
     * STM Timer Initialization (for lwIP timers)
     * ============================================ */
    IfxStm_CompareConfig stmCompareConfig;
    IfxStm_initCompareConfig(&stmCompareConfig);
    stmCompareConfig.triggerPriority = ISR_PRIORITY_OS_TICK;
    stmCompareConfig.comparatorInterrupt = IfxStm_ComparatorInterrupt_ir0;
    stmCompareConfig.ticks = IFX_CFG_STM_TICKS_PER_MS * 10;  /* First interrupt after 10ms */
    stmCompareConfig.typeOfService = IfxSrc_Tos_cpu0;
    IfxStm_initCompare(&MODULE_STM0, &stmCompareConfig);
    
    sendUARTMessage("STM Timer OK\r\n", 14);
    
    /* ============================================
     * GETH Module Initialization
     * ============================================ */
    IfxGeth_enableModule(&MODULE_GETH);
    sendUARTMessage("GETH Module Enabled\r\n", 21);
    
    /* ============================================
     * lwIP Stack Initialization
     * ============================================ */
    /* Define MAC Address */
    eth_addr_t ethAddr;
    ethAddr.addr[0] = 0xDE;
    ethAddr.addr[1] = 0xAD;
    ethAddr.addr[2] = 0xBE;
    ethAddr.addr[3] = 0xEF;
    ethAddr.addr[4] = 0xFE;
    ethAddr.addr[5] = 0xED;
    
    /* Initialize lwIP with MAC address and Static IP */
    Ifx_Lwip_init(ethAddr);
    sendUARTMessage("lwIP Init OK - IP: 192.168.1.10\r\n", 36);
    sendUARTMessage("Ready for Ping Test!\r\n", 22);
    
    /* ============================================
     * TCP Echo Server Initialization
     * ============================================ */
    tcp_echo_server_init();
    
    /* ============================================
     * UDP Echo Server Initialization
     * ============================================ */
    udp_echo_server_init();
    
    /* ============================================
     * Wait for PHY Link UP (5 seconds max)
     * ============================================ */
    sendUARTMessage("Waiting for PHY Link UP...\r\n", 28);
    uint32 phy_wait_start = IfxStm_getLower(&MODULE_STM0);
    uint32 phy_timeout_ticks = (uint32)IfxStm_getTicksFromMilliseconds(&MODULE_STM0, 5000);
    
    while (1)
    {
        Ifx_Lwip_pollTimerFlags();
        Ifx_Lwip_pollReceiveFlags();
        
        /* Check if link is up */
        if (netif_is_link_up(&g_Lwip.netif))
        {
            break;
        }
        
        /* Check timeout */
        if ((IfxStm_get(&MODULE_STM0) - phy_wait_start) > phy_timeout_ticks)
        {
            sendUARTMessage("PHY Link timeout! Continuing anyway...\r\n", 41);
            break;
        }
        
        IfxStm_waitTicks(&MODULE_STM0, IfxStm_getTicksFromMilliseconds(&MODULE_STM0, 100));
    }
    sendUARTMessage("PHY Link UP! Network ready.\r\n", 29);
    
    /* ============================================
     * SPI Flash Initialization (Official iLLD Example Method)
     * ============================================ */
    sendUARTMessage("\r\n", 2);
    sendUARTMessage("***********************************************\r\n", 48);
    sendUARTMessage("*** BUILD: 2025-11-06 SW ROUTING v27    ***\r\n", 48);
    sendUARTMessage("***********************************************\r\n", 48);
    sendUARTMessage("Initializing SPI Flash (S25FL512S)...\r\n", 40);
    sendUARTMessage(">>> QSPI2 Master (Official iLLD Method) <<<\r\n", 45);
    sendUARTMessage("  Method: Like SPI_CPU_1_KIT_TC375_LK\r\n", 39);
    sendUARTMessage("  CS: Hardware Auto (SLSO8_P14_7)\r\n", 35);
    sendUARTMessage("  MISO: pullDown (official method)\r\n", 36);
    sendUARTMessage("  NO explicit CPOL/CPHA (use defaults)\r\n", 40);
    sendUARTMessage("\r\n", 2);
    SPI_Flash_Result flash_result = SPI_Flash_Init(&g_spi_flash);
    if (flash_result == FLASH_OK)
    {
        char msg[64];
        sprintf(msg, "SPI Flash OK: 64MB (ID: 0x%02X%02X%02X)\r\n", 
                g_spi_flash.manufacturer_id,
                g_spi_flash.memory_type,
                g_spi_flash.capacity);
        sendUARTMessage(msg, strlen(msg));
        
        /* Run Flash Tests */
        Test_SPI_Flash();
    }
    else
    {
        char msg[128];
        sprintf(msg, "SPI Flash Init FAILED: %s\r\n", SPI_Flash_GetErrorString(flash_result));
        sendUARTMessage(msg, strlen(msg));
        
        /* Debug: Show what was actually read */
        sprintf(msg, "  Read: 0x%02X 0x%02X 0x%02X (Expected: 0x01 0x02 0x20)\r\n",
                g_spi_flash.manufacturer_id, 
                g_spi_flash.memory_type,
                g_spi_flash.capacity);
        sendUARTMessage(msg, strlen(msg));
        
        sendUARTMessage("  >>> Check mikroBUS connection! <<<\r\n", 38);
        sendUARTMessage("  >>> Flash Click must be properly seated in mikroBUS socket <<<\r\n", 64);
    }
    
    /* ============================================
     * DoIP Client Initialization
     * ============================================ */
    DoIP_ClientConfig doip_config;
    IP4_ADDR(&doip_config.vmg_ip, 192, 168, 1, 100);  /* VMG Simulator IP */
    doip_config.vmg_port = 13400;
    doip_config.source_address = DOIP_ZONAL_GW_ADDRESS;
    
    DoIP_Client_Init(&doip_config);
    sendUARTMessage("[DoIP] Client ready (will connect in 5s)\r\n", 43);
    
    /* ============================================
     * UDS Handler Initialization
     * ============================================ */
    UDS_Init();
    sendUARTMessage("[UDS] Handler initialized\r\n", 27);
    
    /* Initialize UDS Download with SPI Flash */
    UDS_Download_Init(&g_spi_flash);
    sendUARTMessage("[UDS] Download module ready (Flash target)\r\n", 45);
    
    /* Initialize MCU Dual Bank Flash Manager */
    MCU_FlashBank_Init();
    sendUARTMessage("[MCU Bank] Dual Bank Manager ready\r\n", 37);
    
    /* ============================================
     * VCI Initialization (ZG only - Zone ECUs will send via UDP)
     * ============================================ */
    /* Initialize Zonal Gateway VCI (ECU_091) */
    memcpy(g_zgw_vci.ecu_id, ZGW_ECU_ID, sizeof(ZGW_ECU_ID));
    
    /* Try to read software metadata from active MCU bank */
    SoftwarePackageHeader sw_metadata;
    if (SoftwarePackage_ReadActiveMetadata(&sw_metadata))
    {
        /* SUCCESS: Use metadata from MCU PFLASH */
        char version_str[20];
        sprintf(version_str, "%d.%d.%d-%d", 
                sw_metadata.version_major,
                sw_metadata.version_minor,
                sw_metadata.version_patch,
                sw_metadata.version_build);
        
        memcpy(g_zgw_vci.sw_version, version_str, strlen(version_str) + 1);
        
        sendUARTMessage("[VCI] ✓ Loaded SW version from metadata\r\n", 42);
    }
    else
    {
        /* FALLBACK: Use hardcoded version (first boot, or corrupted metadata) */
        memcpy(g_zgw_vci.sw_version, ZGW_SW_VERSION, sizeof(ZGW_SW_VERSION));
        sendUARTMessage("[VCI] Using default SW version (no metadata found)\r\n", 53);
    }
    
    memcpy(g_zgw_vci.hw_version, ZGW_HW_VERSION, sizeof(ZGW_HW_VERSION));
    memcpy(g_zgw_vci.serial_num, ZGW_SERIAL_NUM, sizeof(ZGW_SERIAL_NUM));
    
    sendUARTMessage("\r\n[VCI] Zonal Gateway (ECU_091):\r\n", 34);
    sendUARTMessage("  SW Ver:  ", 11);
    sendUARTMessage(g_zgw_vci.sw_version, strlen(g_zgw_vci.sw_version));
    sendUARTMessage("\r\n  HW Ver:  ", 12);
    sendUARTMessage(g_zgw_vci.hw_version, strlen(g_zgw_vci.hw_version));
    sendUARTMessage("\r\n  Serial:  ", 12);
    sendUARTMessage(g_zgw_vci.serial_num, strlen(g_zgw_vci.serial_num));
    sendUARTMessage("\r\n", 2);
    
    sendUARTMessage("[VCI] Waiting for Zone ECUs to send VCI (0/", 43);
    char max_str[4];
    sprintf(max_str, "%d", MAX_ZONE_ECUS);
    sendUARTMessage(max_str, strlen(max_str));
    sendUARTMessage(")...\r\n", 6);
    
    /* ============================================
     * ECU Health Status Database (for periodic monitoring)
     * ============================================ */
    /* ECU_011: Zone ECU #1 - OK */
    memcpy(g_health_data[0].ecu_id, ZONE_ECU_ID, sizeof(ZONE_ECU_ID));
    g_health_data[0].health_status = HEALTH_STATUS_OK;
    g_health_data[0].dtc_count = 0;
    g_health_data[0].battery_voltage = 1302;   /* 13.02V (0.01V unit) */
    g_health_data[0].temperature = 65;          /* 25°C + 40 */
    
    /* ECU_091: Zonal Gateway - OK */
    memcpy(g_health_data[1].ecu_id, ZGW_ECU_ID, sizeof(ZGW_ECU_ID));
    g_health_data[1].health_status = HEALTH_STATUS_OK;
    g_health_data[1].dtc_count = 0;
    g_health_data[1].battery_voltage = 1320;   /* 13.20V (0.01V unit) */
    g_health_data[1].temperature = 68;          /* 28°C + 40 */
    
    sendUARTMessage("[Health] Status initialized (2 ECUs)\r\n", 39);
    
    boolean vci_report_sent = FALSE;
    uint32 vci_report_wait_start = 0;
    
    boolean health_report_sent = FALSE;
    uint32 health_report_wait_start = 0;
    
    sendUARTMessage("===========================================\r\n", 44);
    sendUARTMessage("Zonal Gateway Ready!\r\n", 22);
    sendUARTMessage("- TCP Echo:    8765\r\n", 21);
    sendUARTMessage("- UDP Echo:    13400\r\n", 22);
    sendUARTMessage("- DoIP Client: VMG @ 192.168.1.100:13400\r\n", 43);
    sendUARTMessage("- VCI:         2 ECUs (ECU_011 + ECU_091)\r\n", 44);
    sendUARTMessage("===========================================\r\n", 44);
    
    /* ============================================
     * Main Loop
     * ============================================ */
    while (1)
    {
        /* Poll lwIP timers and trigger protocol execution if required */
        Ifx_Lwip_pollTimerFlags();
        
        /* Receive data packets through ETH */
        Ifx_Lwip_pollReceiveFlags();
        
        /* Poll DoIP Client */
        DoIP_Client_Poll();
        
        /* Send VCI Report once after DoIP becomes active AND all ECUs reported */
        if (!vci_report_sent && DoIP_Client_IsActive() && g_vci_collection_complete)
        {
            if (vci_report_wait_start == 0)
            {
                /* Start waiting period (500ms after activation) */
                vci_report_wait_start = IfxStm_getLower(&MODULE_STM0);
            }
            else
            {
                /* Wait 500ms before sending VCI report */
                uint32 elapsed_ticks = IfxStm_getLower(&MODULE_STM0) - vci_report_wait_start;
                uint32 wait_ticks = (uint32)IfxStm_getTicksFromMilliseconds(&MODULE_STM0, 500);
                
                if (elapsed_ticks >= wait_ticks)
                {
                    /* Send consolidated VCI (Zone ECUs + ZG) */
                    uint8 total_vci_count = g_zone_ecu_count + 1;  /* +1 for ZG */
                    if (DoIP_Client_SendVCIReport(total_vci_count, g_vci_database))
                    {
                        vci_report_sent = TRUE;
                        sendUARTMessage("[VCI] Consolidated report sent (", 33);
                        char count_str[16];
                        sprintf(count_str, "%d ECUs", total_vci_count);
                        sendUARTMessage(count_str, strlen(count_str));
                        sendUARTMessage(")\r\n", 3);
                    }
                }
            }
        }
        
        /* Send ECU Health Status Report after VCI */
        if (!health_report_sent && vci_report_sent && DoIP_Client_IsActive())
        {
            if (health_report_wait_start == 0)
            {
                /* Start waiting period (1 second after VCI report) */
                health_report_wait_start = IfxStm_getLower(&MODULE_STM0);
            }
            else
            {
                /* Wait 1 second before sending health report */
                uint32 elapsed_ticks = IfxStm_getLower(&MODULE_STM0) - health_report_wait_start;
                uint32 wait_ticks = (uint32)IfxStm_getTicksFromMilliseconds(&MODULE_STM0, 1000);
                
                if (elapsed_ticks >= wait_ticks)
                {
                    if (DoIP_Client_SendHealthStatusReport(2, g_health_data))
                    {
                        health_report_sent = TRUE;
                        sendUARTMessage("[Health] Status report sent (startup)\r\n", 40);
                    }
                }
            }
        }
        
        /* Reset flags if connection lost */
        if ((vci_report_sent || health_report_sent) && !DoIP_Client_IsActive())
        {
            vci_report_sent = FALSE;
            vci_report_wait_start = 0;
            health_report_sent = FALSE;
            health_report_wait_start = 0;
            
            /* Reset VCI collection state */
            g_zone_ecu_count = 0;
            g_vci_collection_complete = FALSE;
            sendUARTMessage("[VCI] Collection reset (connection lost)\r\n", 42);
        }
    }
}
