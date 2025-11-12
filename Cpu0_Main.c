/**********************************************************************************************************************
 * \file Cpu0_Main.c
 * \copyright Copyright (C) Infineon Technologies AG 2019
 * 
 * Zonal Gateway - Main Entry Point
 * Target: TC375 Lite Kit
 *********************************************************************************************************************/
#include "Ifx_Types.h"
#include "IfxCpu.h"
#include "AppConfig.h"
#include "Libraries/DoIP/doip_types.h"
#include "lwip/tcp.h"
#include "lwip/udp.h"
#include "SystemInit.h"
#include "SystemMain.h"

IFX_ALIGN(4) IfxCpu_syncEvent g_cpuSyncEvent = 0;

struct tcp_pcb *g_tcp_server_pcb = NULL;
struct udp_pcb *g_udp_server_pcb = NULL;

DoIP_VCI_Info g_vci_database[MAX_ZONE_ECUS + 1];
uint8 g_zone_ecu_count = 0;
boolean g_vci_collection_complete = FALSE;
DoIP_VCI_Info g_zgw_vci;

DoIP_HealthStatus_Info g_health_data[MAX_ZONE_ECUS + 1];

boolean g_vci_collection_active = FALSE;
uint32 g_vci_collection_start_time = 0;

void core0_main(void)
{
    SystemInit_All();
    SystemMain_Loop();
}

