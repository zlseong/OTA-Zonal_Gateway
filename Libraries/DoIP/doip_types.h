/**
 * @file doip_types.h
 * @brief DoIP Protocol Types and Constants (ISO 13400-2)
 */

#ifndef DOIP_TYPES_H
#define DOIP_TYPES_H

#include "Ifx_Types.h"

/*******************************************************************************
 * DoIP Protocol Constants
 ******************************************************************************/

/* Protocol Version */
#define DOIP_PROTOCOL_VERSION       0x02
#define DOIP_INVERSE_VERSION        0xFD

/* DoIP Header Size */
#define DOIP_HEADER_SIZE            8

/*******************************************************************************
 * DoIP Payload Types (ISO 13400-2)
 ******************************************************************************/

typedef enum
{
    DOIP_GENERIC_NACK               = 0x0000,   /* Generic DoIP header negative acknowledge */
    DOIP_VEHICLE_ID_REQ             = 0x0001,   /* Vehicle identification request */
    DOIP_VEHICLE_ID_REQ_EID         = 0x0002,   /* Vehicle identification request with EID */
    DOIP_VEHICLE_ID_REQ_VIN         = 0x0003,   /* Vehicle identification request with VIN */
    DOIP_VEHICLE_ID_RES             = 0x0004,   /* Vehicle identification response */
    DOIP_ROUTING_ACTIVATION_REQ     = 0x0005,   /* Routing activation request */
    DOIP_ROUTING_ACTIVATION_RES     = 0x0006,   /* Routing activation response */
    DOIP_ALIVE_CHECK_REQ            = 0x0007,   /* Alive check request */
    DOIP_ALIVE_CHECK_RES            = 0x0008,   /* Alive check response */
    DOIP_DIAGNOSTIC_MESSAGE         = 0x8001,   /* DoIP entity status request */
    DOIP_DIAGNOSTIC_MESSAGE_ACK     = 0x8002,   /* Diagnostic message positive acknowledge */
    DOIP_DIAGNOSTIC_MESSAGE_NACK    = 0x8003,   /* Diagnostic message negative acknowledge */
    
    /* Custom payload types */
    DOIP_VCI_REPORT                 = 0x9000,   /* VCI (Vehicle Configuration Information) report */
    DOIP_HEALTH_STATUS_REPORT       = 0x9001    /* ECU Health Status report (Dynamic monitoring) */
    
} DoIP_PayloadType;

/*******************************************************************************
 * DoIP Routing Activation Response Codes
 ******************************************************************************/

typedef enum
{
    DOIP_RA_RES_UNKNOWN             = 0x00,     /* Unknown source address */
    DOIP_RA_RES_SUCCESS             = 0x10,     /* Routing activation successful */
    DOIP_RA_RES_CONFIRMATION_REQ    = 0x11,     /* Confirmation required */
    DOIP_RA_RES_DENIED_UNKNOWN_SA   = 0x00,     /* Denied - unknown source address */
    DOIP_RA_RES_DENIED_NO_SOCKET    = 0x01,     /* Denied - no socket available */
    DOIP_RA_RES_DENIED_SA_IN_USE    = 0x02,     /* Denied - source address already in use */
    DOIP_RA_RES_DENIED_SA_DIFF      = 0x03,     /* Denied - source address different */
    DOIP_RA_RES_DENIED_ACTIVE       = 0x04,     /* Denied - already active */
    DOIP_RA_RES_DENIED_AUTH_MISSING = 0x05,     /* Denied - authentication missing */
    DOIP_RA_RES_DENIED_CONFIRM_REJ  = 0x06      /* Denied - confirmation rejected */
    
} DoIP_RoutingActivationResponse;

/*******************************************************************************
 * DoIP Client States
 ******************************************************************************/

typedef enum
{
    DOIP_STATE_IDLE,                /* Not connected */
    DOIP_STATE_CONNECTING,          /* TCP connection in progress */
    DOIP_STATE_CONNECTED,           /* TCP connected, waiting for routing activation */
    DOIP_STATE_ACTIVE,              /* Routing activated, ready for communication */
    DOIP_STATE_ERROR                /* Error state */
    
} DoIP_ClientState;

/*******************************************************************************
 * DoIP Message Header Structure
 ******************************************************************************/

typedef struct
{
    uint8  protocolVersion;         /* Protocol version (0x02) */
    uint8  inverseProtocolVersion;  /* Inverse protocol version (0xFD) */
    uint16 payloadType;             /* Payload type */
    uint32 payloadLength;           /* Payload length */
    
} DoIP_Header;

/*******************************************************************************
 * DoIP Configuration
 ******************************************************************************/

/* Timeouts (milliseconds) */
#define DOIP_TIMEOUT_CONNECTION     3000    /* TCP connection timeout: 3 seconds */
#define DOIP_TIMEOUT_ROUTING        2000    /* Routing activation response timeout: 2 seconds */
#define DOIP_TIMEOUT_ALIVE_CHECK    500     /* Alive check response timeout: 500ms */
#define DOIP_RECONNECT_INTERVAL     5000    /* Reconnection interval: 5 seconds */

/* Alive Check Configuration */
#define DOIP_ALIVE_CHECK_INTERVAL   5000    /* Alive check interval: 5 seconds */

/* Buffer Sizes */
#define DOIP_MAX_MESSAGE_SIZE       256     /* Maximum DoIP message size */
#define DOIP_TX_BUFFER_SIZE         256     /* Transmit buffer size */
#define DOIP_RX_BUFFER_SIZE         256     /* Receive buffer size */

/* Logical Addresses */
#define DOIP_ZONAL_GW_ADDRESS       0x0100  /* Zonal Gateway logical address */
#define DOIP_VMG_ADDRESS            0x0200  /* VMG logical address */

/*******************************************************************************
 * VCI (Vehicle Configuration Information) Structures
 ******************************************************************************/

/* VCI Information for a single ECU */
typedef struct
{
    char ecu_id[16];        /* ECU ID (e.g., "ECU_091") */
    char sw_version[8];     /* Software version (e.g., "0.0.0") */
    char hw_version[8];     /* Hardware version (e.g., "0.0.0") */
    char serial_num[16];    /* Serial number (e.g., "091000001") */
} DoIP_VCI_Info;

/* Zonal Gateway VCI Constants (ECU_09x format) */
#define ZGW_ECU_ID          "ECU_091"      /* Zonal Gateway #1 */
#define ZGW_SW_VERSION      "0.0.0"
#define ZGW_HW_VERSION      "0.0.0"
#define ZGW_SERIAL_NUM      "091000001"

/* Zone ECU VCI Constants (ECU_0xy format) */
#define ZONE_ECU_ID         "ECU_011"      /* Zone 1, ECU #1 */
#define ZONE_ECU_SW_VER     "0.0.0"
#define ZONE_ECU_HW_VER     "0.0.0"
#define ZONE_ECU_SERIAL     "011000001"

/* VCI Collection Configuration */
#define MAX_ZONE_ECUS       1              /* Maximum ECUs per zone */
#define VCI_COLLECTION_TIMEOUT  5000       /* VCI collection timeout: 5 seconds */

/*******************************************************************************
 * ECU Health Status Structures (Dynamic Monitoring)
 ******************************************************************************/

/* ECU Health Status Codes */
typedef enum
{
    HEALTH_STATUS_OK        = 0x00,     /* Normal operation */
    HEALTH_STATUS_WARNING   = 0x01,     /* Warning condition */
    HEALTH_STATUS_ERROR     = 0x02,     /* Error condition */
    HEALTH_STATUS_CRITICAL  = 0x03      /* Critical failure */
} ECU_HealthStatus;

/* ECU Health Information for a single ECU */
typedef struct
{
    char ecu_id[16];            /* ECU ID (e.g., "ECU_091") */
    uint8 health_status;        /* Health status code (ECU_HealthStatus) */
    uint8 dtc_count;            /* Number of active DTCs */
    uint16 battery_voltage;     /* Battery voltage (mV, e.g., 13020 = 13.02V) */
    uint8 temperature;          /* ECU temperature (°C + 40 offset, e.g., 65 = 25°C) */
    uint8 reserved[3];          /* Reserved for future use */
} DoIP_HealthStatus_Info;

/* Health Status Configuration */
#define HEALTH_CHECK_INTERVAL   10000   /* Health check interval: 10 seconds */

#endif /* DOIP_TYPES_H */

