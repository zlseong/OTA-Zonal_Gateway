/**
 * @file SystemInit.c
 * @brief System Initialization Manager Implementation
 */

#include "SystemInit.h"
#include "Ifx_Types.h"
#include "IfxCpu.h"
#include "IfxScuWdt.h"
#include "IfxStm.h"
#include "IfxGeth_Eth.h"
#include "Ifx_Lwip.h"
#include "AppConfig.h"
#include "Configuration.h"
#include "ConfigurationIsr.h"
#include "UART_Logging.h"
#include "lwip/tcp.h"
#include "lwip/udp.h"
#include "lwip/pbuf.h"
#include "Libraries/DoIP/doip_client.h"
#include "Libraries/DoIP/uds_handler.h"
#include "Flash4_Driver.h"
#include "Flash4_Test.h"
#include "TcpEchoServer.h"
#include "UdpEchoServer.h"
#include "Libraries/DataCollection/vci_manager.h"
#include "Libraries/DataCollection/readiness_manager.h"
#include <string.h>
#include <stdio.h>

extern IFX_ALIGN(4) IfxCpu_syncEvent g_cpuSyncEvent;
extern struct tcp_pcb *g_tcp_server_pcb;
extern struct udp_pcb *g_udp_server_pcb;
extern DoIP_VCI_Info g_zgw_vci;
extern DoIP_HealthStatus_Info g_health_data[MAX_ZONE_ECUS + 1];

static void Init_System(void);
static void Init_STM_Timer(void);
static void Init_Ethernet(void);
static void Wait_PHY_Link(void);
static void Init_DoIP(void);
static void Init_VCI(void);
static void Init_Readiness(void);
static void Init_Health_Database(void);
static void Print_System_Ready(void);

static void Init_System(void)
{
    IfxCpu_enableInterrupts();
    IfxScuWdt_disableCpuWatchdog(IfxScuWdt_getCpuWatchdogPassword());
    IfxScuWdt_disableSafetyWatchdog(IfxScuWdt_getSafetyWatchdogPassword());
    IfxCpu_emitEvent(&g_cpuSyncEvent);
    IfxCpu_waitEvent(&g_cpuSyncEvent, 1);
}

static void Init_STM_Timer(void)
{
    IfxStm_CompareConfig stmCompareConfig;
    IfxStm_initCompareConfig(&stmCompareConfig);
    stmCompareConfig.triggerPriority = ISR_PRIORITY_OS_TICK;
    stmCompareConfig.comparatorInterrupt = IfxStm_ComparatorInterrupt_ir0;
    stmCompareConfig.ticks = IFX_CFG_STM_TICKS_PER_MS * 10;
    stmCompareConfig.typeOfService = IfxSrc_Tos_cpu0;
    IfxStm_initCompare(&MODULE_STM0, &stmCompareConfig);
    sendUARTMessage("STM Timer OK\r\n", 14);
}

static void Init_Ethernet(void)
{
    IfxGeth_enableModule(&MODULE_GETH);
    sendUARTMessage("GETH Module Enabled\r\n", 21);
    
    eth_addr_t ethAddr;
    ethAddr.addr[0] = ETH_MAC_ADDR_0;
    ethAddr.addr[1] = ETH_MAC_ADDR_1;
    ethAddr.addr[2] = ETH_MAC_ADDR_2;
    ethAddr.addr[3] = ETH_MAC_ADDR_3;
    ethAddr.addr[4] = ETH_MAC_ADDR_4;
    ethAddr.addr[5] = ETH_MAC_ADDR_5;
    
    Ifx_Lwip_init(ethAddr);
    sendUARTMessage("lwIP Init OK - IP: 192.168.1.10\r\n", 36);
    sendUARTMessage("Ready for Ping Test!\r\n", 22);
}

static void Wait_PHY_Link(void)
{
    sendUARTMessage("Waiting for PHY Link UP...\r\n", 28);
    uint32 phy_wait_start = IfxStm_getLower(&MODULE_STM0);
    uint32 phy_timeout_ticks = (uint32)IfxStm_getTicksFromMilliseconds(&MODULE_STM0, PHY_LINK_TIMEOUT_MS);
    
    while (1)
    {
        Ifx_Lwip_pollTimerFlags();
        Ifx_Lwip_pollReceiveFlags();
        
        if (netif_is_link_up(&g_Lwip.netif))
        {
            break;
        }
        
        if ((IfxStm_get(&MODULE_STM0) - phy_wait_start) > phy_timeout_ticks)
        {
            sendUARTMessage("PHY Link timeout! Continuing anyway...\r\n", 41);
            break;
        }
        
        IfxStm_waitTicks(&MODULE_STM0, IfxStm_getTicksFromMilliseconds(&MODULE_STM0, 100));
    }
    sendUARTMessage("PHY Link UP! Network ready.\r\n", 29);
}

static void Init_DoIP(void)
{
    DoIP_ClientConfig doip_config;
    IP4_ADDR(&doip_config.vmg_ip, VMG_IP_ADDR_0, VMG_IP_ADDR_1, VMG_IP_ADDR_2, VMG_IP_ADDR_3);
    doip_config.vmg_port = VMG_PORT;
    doip_config.source_address = DOIP_ZONAL_GW_ADDRESS;
    DoIP_Client_Init(&doip_config);
    sendUARTMessage("[DoIP] Client ready (will connect in 5s)\r\n", 43);
    
    UDS_Init();
    sendUARTMessage("[UDS] Handler initialized\r\n", 27);
}

static void Init_VCI(void)
{
    memcpy(g_zgw_vci.ecu_id, ZGW_ECU_ID, sizeof(ZGW_ECU_ID));
    memcpy(g_zgw_vci.sw_version, ZGW_SW_VERSION, sizeof(ZGW_SW_VERSION));
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
}

static void Init_Readiness(void)
{
    Readiness_Init();
    sendUARTMessage("[Readiness] Manager initialized\r\n", 34);
}

static void Init_Health_Database(void)
{
    memcpy(g_health_data[0].ecu_id, ZONE_ECU_ID, sizeof(ZONE_ECU_ID));
    g_health_data[0].health_status = HEALTH_STATUS_OK;
    g_health_data[0].dtc_count = 0;
    g_health_data[0].battery_voltage = 1302;
    g_health_data[0].temperature = 65;
    
    memcpy(g_health_data[1].ecu_id, ZGW_ECU_ID, sizeof(ZGW_ECU_ID));
    g_health_data[1].health_status = HEALTH_STATUS_OK;
    g_health_data[1].dtc_count = 0;
    g_health_data[1].battery_voltage = 1320;
    g_health_data[1].temperature = 68;
    
    sendUARTMessage("[Health] Status initialized (2 ECUs)\r\n", 39);
}

static void Print_System_Ready(void)
{
    sendUARTMessage("===========================================\r\n", 44);
    sendUARTMessage("Zonal Gateway Ready!\r\n", 22);
    sendUARTMessage("- TCP Echo:    8765\r\n", 21);
    sendUARTMessage("- UDP Echo:    13400\r\n", 22);
    sendUARTMessage("- DoIP Client: VMG @ 192.168.1.100:13400\r\n", 43);
    sendUARTMessage("- VCI:         Command-based (use UDS 0x31)\r\n", 46);
    sendUARTMessage("  * 0x31 01 F001: Start VCI collection\r\n", 40);
    sendUARTMessage("  * 0x31 01 F002: Send VCI report\r\n", 34);
    sendUARTMessage("  * 0x31 01 F003: Start Readiness check\r\n", 41);
    sendUARTMessage("  * 0x31 01 F004: Send Readiness report\r\n", 41);
    sendUARTMessage("===========================================\r\n", 44);
}

void SystemInit_All(void)
{
    Init_System();
    
    initUART();
    sendUARTMessage("Zonal Gateway Starting...\r\n", 28);
    
    Init_STM_Timer();
    Flash4_Init();
    Test_Flash4();
    Init_Ethernet();
    
    tcp_echo_server_init();
    udp_echo_server_init();
    
    Wait_PHY_Link();
    Init_DoIP();
    Init_VCI();
    Init_Readiness();
    Init_Health_Database();
    Print_System_Ready();
}

