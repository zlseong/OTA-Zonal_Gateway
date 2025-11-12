/**********************************************************************************************************************
 * \file TcpEchoServer.c
 * \copyright Copyright (C) Infineon Technologies AG 2019
 * 
 * TCP Echo Server Implementation
 *********************************************************************************************************************/

#include "TcpEchoServer.h"
#include "AppConfig.h"
#include "UART_Logging.h"
#include "lwip/tcp.h"
#include "lwip/pbuf.h"
#include <string.h>
#include <stdio.h>

extern struct tcp_pcb *g_tcp_server_pcb;

static err_t tcp_echo_recv_callback(void *arg, struct tcp_pcb *tpcb, struct pbuf *p, err_t err)
{
    err_t ret_err;
    
    if (p == NULL)
    {
        sendUARTMessage("TCP Client Disconnected\r\n", 26);
        tcp_close(tpcb);
        return ERR_OK;
    }
    
    tcp_recved(tpcb, p->tot_len);
    ret_err = tcp_write(tpcb, p->payload, p->tot_len, TCP_WRITE_FLAG_COPY);
    
    if (ret_err == ERR_OK)
    {
        tcp_output(tpcb);
        char log[64];
        sprintf(log, "TCP Echo: %d bytes\r\n", p->tot_len);
        sendUARTMessage(log, strlen(log));
    }
    
    pbuf_free(p);
    return ERR_OK;
}

static void tcp_echo_error_callback(void *arg, err_t err)
{
    sendUARTMessage("TCP Error\r\n", 11);
}

static err_t tcp_echo_accept_callback(void *arg, struct tcp_pcb *newpcb, err_t err)
{
    sendUARTMessage("TCP Client Connected\r\n", 22);
    tcp_recv(newpcb, tcp_echo_recv_callback);
    tcp_err(newpcb, tcp_echo_error_callback);
    return ERR_OK;
}

void tcp_echo_server_init(void)
{
    err_t err;
    
    g_tcp_server_pcb = tcp_new();
    if (g_tcp_server_pcb == NULL)
    {
        sendUARTMessage("TCP PCB creation failed\r\n", 26);
        return;
    }
    
    err = tcp_bind(g_tcp_server_pcb, IP_ADDR_ANY, TCP_ECHO_PORT);
    if (err != ERR_OK)
    {
        sendUARTMessage("TCP Bind failed\r\n", 17);
        tcp_close(g_tcp_server_pcb);
        return;
    }
    
    g_tcp_server_pcb = tcp_listen(g_tcp_server_pcb);
    if (g_tcp_server_pcb == NULL)
    {
        sendUARTMessage("TCP Listen failed\r\n", 19);
        return;
    }
    
    tcp_accept(g_tcp_server_pcb, tcp_echo_accept_callback);
    sendUARTMessage("TCP Echo Server started on port 8765\r\n", 38);
}

