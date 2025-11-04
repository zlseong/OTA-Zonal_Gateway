/**
 * @file doip_client.h
 * @brief DoIP Client for Zonal Gateway
 */

#ifndef DOIP_CLIENT_H
#define DOIP_CLIENT_H

#include "doip_types.h"
#include "lwip/tcp.h"
#include "Ifx_Types.h"

/*******************************************************************************
 * DoIP Client Configuration Structure
 ******************************************************************************/

typedef struct
{
    ip_addr_t vmg_ip;               /* VMG IP address */
    uint16    vmg_port;             /* VMG port (default: 13400) */
    uint16    source_address;       /* Zonal Gateway logical address */
    
} DoIP_ClientConfig;

/*******************************************************************************
 * Function Prototypes
 ******************************************************************************/

/**
 * @brief Initialize DoIP Client
 * @param config Configuration structure
 */
void DoIP_Client_Init(const DoIP_ClientConfig *config);

/**
 * @brief Poll DoIP Client (call periodically from main loop)
 * Must be called every few milliseconds
 */
void DoIP_Client_Poll(void);

/**
 * @brief Get current DoIP client state
 * @return Current state
 */
DoIP_ClientState DoIP_Client_GetState(void);

/**
 * @brief Check if DoIP client is active (routing activated)
 * @return TRUE if active, FALSE otherwise
 */
boolean DoIP_Client_IsActive(void);

/**
 * @brief Send ECU Health Status Report
 * @param ecu_count Number of ECUs
 * @param health_data Array of DoIP_HealthStatus_Info structures
 * @return TRUE if sent successfully, FALSE otherwise
 */
boolean DoIP_Client_SendHealthStatusReport(uint8 ecu_count, const DoIP_HealthStatus_Info *health_data);

/**
 * @brief Send VCI (Vehicle Configuration Information) Report to VMG
 * @param vci_count Number of ECUs in VCI report
 * @param vci_database Array of VCI_Info structures
 * @return TRUE if sent successfully, FALSE otherwise
 */
boolean DoIP_Client_SendVCIReport(uint8 vci_count, const DoIP_VCI_Info *vci_database);

/**
 * @brief Close DoIP connection
 */
void DoIP_Client_Close(void);

#endif /* DOIP_CLIENT_H */

