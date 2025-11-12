/**********************************************************************************************************************
 * \file vci_manager.c
 * \copyright Copyright (C) Infineon Technologies AG 2019
 * 
 * VCI Collection Manager Implementation
 *********************************************************************************************************************/

#include "vci_manager.h"
#include "AppConfig.h"
#include "UART_Logging.h"
#include "Libraries/DoIP/doip_types.h"
#include "IfxStm.h"
#include "lwip/udp.h"
#include "lwip/pbuf.h"
#include <string.h>
#include <stdio.h>

extern struct udp_pcb *g_udp_server_pcb;
extern DoIP_VCI_Info g_vci_database[MAX_ZONE_ECUS + 1];
extern uint8 g_zone_ecu_count;
extern boolean g_vci_collection_complete;
extern DoIP_VCI_Info g_zgw_vci;
extern boolean g_vci_collection_active;
extern uint32 g_vci_collection_start_time;

/**
 * @brief Send UDP broadcast to request VCI from all Zone ECUs
 * 
 * Sends a simple VCI request packet (magic number) to broadcast address
 * Zone ECUs listening on UDP 13400 will respond with their VCI
 */
void VCI_SendCollectionRequest(void)
{
    if (g_udp_server_pcb == NULL)
    {
        sendUARTMessage("[VCI] UDP not ready\r\n", 21);
        return;
    }
    
    /* Prepare VCI request packet: [Magic: "RQST"] */
    uint8 request[4] = {0x52, 0x51, 0x53, 0x54};  /* "RQST" */
    
    /* Create pbuf for request */
    struct pbuf *p = pbuf_alloc(PBUF_TRANSPORT, 4, PBUF_RAM);
    if (p == NULL)
    {
        sendUARTMessage("[VCI] pbuf alloc failed\r\n", 25);
        return;
    }
    
    memcpy(p->payload, request, 4);
    
    /* Broadcast address: 192.168.1.255 */
    ip_addr_t broadcast_addr;
    IP4_ADDR(&broadcast_addr, 192, 168, 1, 255);
    
    /* Send to broadcast address */
    err_t err = udp_sendto(g_udp_server_pcb, p, &broadcast_addr, UDP_DOIP_PORT);
    
    pbuf_free(p);
    
    if (err == ERR_OK)
    {
        sendUARTMessage("[VCI] Broadcast request sent (192.168.1.255:13400)\r\n", 53);
    }
    else
    {
        char msg[64];
        sprintf(msg, "[VCI] Broadcast failed: err=%d\r\n", err);
        sendUARTMessage(msg, strlen(msg));
    }
}

/**
 * @brief Start VCI collection from Zone ECUs
 * Called by UDS Routine Control (0x31 01 F001)
 */
void VCI_StartCollection(void)
{
    /* Reset VCI database */
    g_zone_ecu_count = 0;
    g_vci_collection_complete = FALSE;
    
    /* Re-add ZGW VCI to database at index 0 */
    memcpy(&g_vci_database[0], &g_zgw_vci, sizeof(DoIP_VCI_Info));
    
    /* Start collection timer */
    g_vci_collection_active = TRUE;
    g_vci_collection_start_time = IfxStm_getLower(&MODULE_STM0);
    
    /* Send broadcast request */
    VCI_SendCollectionRequest();
    
    sendUARTMessage("[VCI] Collection started (10s timeout)\r\n", 40);
}

/**
 * @brief Check VCI collection timeout in main loop
 */
void VCI_CheckCollectionTimeout(void)
{
    if (!g_vci_collection_active || g_vci_collection_complete)
    {
        return;
    }
    
    /* Check timeout */
    uint32 elapsed_ticks = IfxStm_getLower(&MODULE_STM0) - g_vci_collection_start_time;
    uint32 timeout_ticks = (uint32)IfxStm_getTicksFromMilliseconds(&MODULE_STM0, VCI_COLLECTION_TIMEOUT_MS);
    
    if (elapsed_ticks >= timeout_ticks)
    {
        /* Timeout reached - finalize collection with current ECUs */
        g_vci_collection_complete = TRUE;
        g_vci_collection_active = FALSE;
        
        /* Add ZG's VCI to the end */
        memcpy(&g_vci_database[g_zone_ecu_count], &g_zgw_vci, sizeof(DoIP_VCI_Info));
        
        char msg[64];
        sprintf(msg, "[VCI] Collection timeout (%d Zone ECUs + ZGW)\r\n", g_zone_ecu_count);
        sendUARTMessage(msg, strlen(msg));
    }
}

