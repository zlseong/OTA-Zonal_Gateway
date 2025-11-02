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
#include <string.h>
#include <stdio.h>

IFX_ALIGN(4) IfxCpu_syncEvent g_cpuSyncEvent = 0;

/* TCP Echo Server (포트 8765) */
#define TCP_ECHO_PORT  8765

/* UDP Echo Server (포트 13400) */
#define UDP_DOIP_PORT  13400

static struct tcp_pcb *g_tcp_server_pcb = NULL;
static struct udp_pcb *g_udp_server_pcb = NULL;

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

/* UDP 데이터 수신 콜백 (Echo: 받은 데이터를 그대로 전송) */
static void udp_echo_recv_callback(void *arg, struct udp_pcb *upcb, struct pbuf *p,
                                   const ip_addr_t *addr, u16_t port)
{
    if (p == NULL) {
        return;
    }
    
    /* Echo: 받은 데이터를 그대로 전송 */
    udp_sendto(upcb, p, addr, port);
    
    /* UART 로그 */
    char log[64];
    sprintf(log, "UDP Echo: %d bytes from %s:%d\r\n", 
            p->tot_len, ip4addr_ntoa(addr), port);
    sendUARTMessage(log, strlen(log));
    
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
     * Main Loop
     * ============================================ */
    while (1)
    {
        /* Poll lwIP timers and trigger protocol execution if required */
        Ifx_Lwip_pollTimerFlags();
        
        /* Receive data packets through ETH */
        Ifx_Lwip_pollReceiveFlags();
    }
}
