/**********************************************************************************************************************
 * \file readiness_manager.c
 * \copyright Copyright (C) Infineon Technologies AG 2019
 * 
 * Readiness Collection Manager - Implementation
 *********************************************************************************************************************/

#include "readiness_manager.h"
#include "Libraries/DoIP/doip_types.h"
#include "Libraries/DoIP/doip_client.h"
#include "UART_Logging.h"
#include "IfxStm.h"
#include "lwip/udp.h"
#include "lwip/pbuf.h"
#include <string.h>
#include <stdio.h>

/*******************************************************************************
 * External Variables
 ******************************************************************************/

/* ZGW VCI (defined in Cpu0_Main.c) */
extern DoIP_VCI_Info g_zgw_vci;

/* UDP server PCB (defined in Cpu0_Main.c) */
extern struct udp_pcb *g_udp_server_pcb;

/*******************************************************************************
 * Private Variables
 ******************************************************************************/

/* Readiness Database */
static Readiness_Info g_readiness_database[MAX_READINESS_ECUS + 1];  /* +1 for ZGW itself */
static uint8 g_readiness_ecu_count = 0;

/* Collection State */
static boolean g_readiness_collection_active = FALSE;
static boolean g_readiness_collection_complete = FALSE;
static uint32 g_readiness_check_start_time = 0;

/*******************************************************************************
 * Private Function Prototypes
 ******************************************************************************/

static void Readiness_SendRequest(void);
static boolean Check_VehicleParked(void);
static boolean Check_EngineOff(void);
static uint16 Get_BatteryVoltage_mV(void);
static uint32 Get_AvailableFlash_KB(void);
static boolean Check_AllDoorsClosed(void);
static boolean Check_SW_Compatibility(void);

/*******************************************************************************
 * Public Functions
 ******************************************************************************/

static void Readiness_SendRequest(void)
{
    if (g_udp_server_pcb == NULL)
    {
        sendUARTMessage("[Readiness] UDP not ready\r\n", 28);
        return;
    }
    
    /* Prepare Readiness request packet: [0xAA 0xBB 0x03 0x00] */
    uint8 request[4] = {0xAA, 0xBB, 0x03, 0x00};
    
    /* Create pbuf for request */
    struct pbuf *p = pbuf_alloc(PBUF_TRANSPORT, 4, PBUF_RAM);
    if (p == NULL)
    {
        sendUARTMessage("[Readiness] pbuf alloc failed\r\n", 33);
        return;
    }
    
    memcpy(p->payload, request, 4);
    
    /* Broadcast address: 192.168.1.255 */
    ip_addr_t broadcast_addr;
    IP4_ADDR(&broadcast_addr, 192, 168, 1, 255);
    
    /* Send to broadcast address */
    err_t err = udp_sendto(g_udp_server_pcb, p, &broadcast_addr, 13400);
    
    pbuf_free(p);
    
    if (err == ERR_OK)
    {
        sendUARTMessage("[Readiness] Broadcast sent (192.168.1.255:13400)\r\n", 51);
    }
    else
    {
        char msg[64];
        sprintf(msg, "[Readiness] Broadcast failed: err=%d\r\n", err);
        sendUARTMessage(msg, strlen(msg));
    }
}

void Readiness_Init(void)
{
    sendUARTMessage("[Readiness] Manager initialized\r\n", 34);
    
    memset(g_readiness_database, 0, sizeof(g_readiness_database));
    g_readiness_ecu_count = 0;
    g_readiness_collection_active = FALSE;
    g_readiness_collection_complete = FALSE;
}

void Readiness_StartCheck(void)
{
    sendUARTMessage("[Readiness] Starting Check (UDP Broadcast)...\r\n", 48);
    
    /* Reset database */
    memset(g_readiness_database, 0, sizeof(g_readiness_database));
    g_readiness_ecu_count = 0;
    
    /* Add ZGW's own readiness first */
    g_readiness_database[0] = Readiness_GetLocal();
    
    char msg[128];
    sprintf(msg, "[Readiness] ZGW: Ready=%u, Battery=%umV, Memory=%uKB\r\n",
            (unsigned int)g_readiness_database[0].ready_for_update,
            (unsigned int)g_readiness_database[0].battery_voltage_mv,
            (unsigned int)g_readiness_database[0].available_memory_kb);
    sendUARTMessage(msg, strlen(msg));
    
    /* Send UDP broadcast to Zone ECUs (0xAA 0xBB 0x03 0x00) */
    Readiness_SendRequest();
    
    /* Start collection timer */
    g_readiness_check_start_time = IfxStm_get(&MODULE_STM0);
    g_readiness_collection_active = TRUE;
    g_readiness_collection_complete = FALSE;
    
    sendUARTMessage("[Readiness] Waiting for Zone ECUs...\r\n", 39);
}

boolean Readiness_IsCheckComplete(void)
{
    return g_readiness_collection_complete;
}

void Readiness_CheckTimeout(void)
{
    if (!g_readiness_collection_active || g_readiness_collection_complete)
    {
        return;
    }
    
    /* Check timeout */
    uint32 current_time = IfxStm_get(&MODULE_STM0);
    uint32 elapsed_ticks = current_time - g_readiness_check_start_time;
    uint32 elapsed_ms = IfxStm_getTicksFromMilliseconds(&MODULE_STM0, elapsed_ticks);
    
    if (elapsed_ms >= READINESS_COLLECTION_TIMEOUT)
    {
        g_readiness_collection_complete = TRUE;
        g_readiness_collection_active = FALSE;
        
        char msg[64];
        sprintf(msg, "[Readiness] Check complete (%u ECUs)\r\n", (unsigned int)(g_readiness_ecu_count + 1));
        sendUARTMessage(msg, strlen(msg));
    }
}

boolean Readiness_GetConsolidated(Readiness_Info* readiness_array, uint8* readiness_count)
{
    if (readiness_array == NULL || readiness_count == NULL)
    {
        return FALSE;
    }
    
    /* Check if readiness check is complete */
    if (!g_readiness_collection_complete)
    {
        /* Return only ZGW readiness */
        readiness_array[0] = Readiness_GetLocal();
        *readiness_count = 1;
        return TRUE;
    }
    
    /* Return all collected readiness (Zone ECUs + ZGW) */
    uint8 total_count = g_readiness_ecu_count + 1;  /* +1 for ZGW itself */
    
    if (total_count > MAX_READINESS_ECUS + 1)
    {
        total_count = MAX_READINESS_ECUS + 1;
    }
    
    memcpy(readiness_array, g_readiness_database, total_count * sizeof(Readiness_Info));
    *readiness_count = total_count;
    
    return TRUE;
}

Readiness_Info Readiness_GetLocal(void)
{
    Readiness_Info info;
    
    /* ECU ID */
    strcpy(info.ecu_id, g_zgw_vci.ecu_id);
    
    /* Check conditions */
    info.vehicle_parked = Check_VehicleParked();
    info.engine_off = Check_EngineOff();
    info.battery_voltage_mv = Get_BatteryVoltage_mV();
    info.available_memory_kb = Get_AvailableFlash_KB();
    info.all_doors_closed = Check_AllDoorsClosed();
    
    /* Check compatibility */
    info.compatible = Check_SW_Compatibility();
    
    /* Final decision - ALL conditions must be TRUE */
    info.ready_for_update = (
        info.vehicle_parked &&
        info.engine_off &&
        info.battery_voltage_mv >= 12500 &&  /* Minimum 12.5V */
        info.available_memory_kb >= 3072 &&  /* Minimum 3MB free */
        info.all_doors_closed &&
        info.compatible
    );
    
    return info;
}

void Readiness_HandleResponse(const Readiness_Info* readiness_info)
{
    if (readiness_info == NULL || !g_readiness_collection_active)
    {
        return;
    }
    
    /* Check if ECU already exists */
    for (uint8 i = 0; i <= g_readiness_ecu_count; i++)
    {
        if (strcmp(g_readiness_database[i].ecu_id, readiness_info->ecu_id) == 0)
        {
            /* Update existing entry */
            g_readiness_database[i] = *readiness_info;
            
            char msg[80];
            sprintf(msg, "[Readiness] Updated %s (Ready=%u)\r\n", 
                    readiness_info->ecu_id, 
                    (unsigned int)readiness_info->ready_for_update);
            sendUARTMessage(msg, strlen(msg));
            return;
        }
    }
    
    /* Add new ECU */
    if (g_readiness_ecu_count < MAX_READINESS_ECUS)
    {
        g_readiness_ecu_count++;
        g_readiness_database[g_readiness_ecu_count] = *readiness_info;
        
        char msg[80];
        sprintf(msg, "[Readiness] Added %s (%u/%u ECUs)\r\n",
                readiness_info->ecu_id,
                (unsigned int)(g_readiness_ecu_count + 1),  /* +1 for ZGW */
                (unsigned int)(MAX_READINESS_ECUS + 1));
        sendUARTMessage(msg, strlen(msg));
    }
}

/*******************************************************************************
 * Private Functions - Condition Checks
 ******************************************************************************/

static boolean Check_VehicleParked(void)
{
    /* TODO: Read gear position from CAN or GPIO */
    /* For now, return TRUE (simulated) */
    return TRUE;
}

static boolean Check_EngineOff(void)
{
    /* TODO: Read engine state from CAN */
    /* For now, return TRUE (simulated) */
    return TRUE;
}

static uint16 Get_BatteryVoltage_mV(void)
{
    /* TODO: Read battery voltage from ADC */
    /* For now, return simulated value: 13.2V */
    return 13200;  /* mV */
}

static uint32 Get_AvailableFlash_KB(void)
{
    /* TODO: Calculate available PFlash from flash manager */
    /* Bank B size: 3 MB = 3072 KB */
    /* For now, return simulated value */
    return 3072;  /* KB */
}

static boolean Check_AllDoorsClosed(void)
{
    /* TODO: Read door status from CAN */
    /* For now, return TRUE (simulated) */
    return TRUE;
}

static boolean Check_SW_Compatibility(void)
{
    /* TODO: Check SW compatibility based on VCI and metadata */
    /* For now, return TRUE (simulated) */
    return TRUE;
}

