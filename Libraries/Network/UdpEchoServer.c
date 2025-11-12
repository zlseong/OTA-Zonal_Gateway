/**********************************************************************************************************************
 * \file UdpEchoServer.c
 * \copyright Copyright (C) Infineon Technologies AG 2019
 * 
 * UDP Echo Server Implementation (with VCI Message Reception)
 *********************************************************************************************************************/

#include "UdpEchoServer.h"
#include "AppConfig.h"
#include "UART_Logging.h"
#include "Libraries/DoIP/doip_types.h"
#include "lwip/udp.h"
#include "lwip/pbuf.h"
#include <string.h>
#include <stdio.h>

extern struct udp_pcb *g_udp_server_pcb;
extern DoIP_VCI_Info g_vci_database[MAX_ZONE_ECUS + 1];
extern uint8 g_zone_ecu_count;
extern boolean g_vci_collection_complete;
extern DoIP_VCI_Info g_zgw_vci;

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
    
    pbuf_free(p);
}

void udp_echo_server_init(void)
{
    err_t err;
    
    g_udp_server_pcb = udp_new();
    if (g_udp_server_pcb == NULL) {
        sendUARTMessage("UDP PCB creation failed\r\n", 26);
        return;
    }
    
    err = udp_bind(g_udp_server_pcb, IP_ADDR_ANY, UDP_DOIP_PORT);
    if (err != ERR_OK) {
        sendUARTMessage("UDP Bind failed\r\n", 17);
        udp_remove(g_udp_server_pcb);
        return;
    }
    
    udp_recv(g_udp_server_pcb, udp_echo_recv_callback, NULL);
    
    sendUARTMessage("UDP Echo Server started on port 13400\r\n", 40);
}

