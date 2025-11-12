/**
 * @file doip_client.c
 * @brief DoIP Client Implementation
 */

#include "doip_client.h"
#include "doip_message.h"
#include "uds_handler.h"
#include "Ifx_Lwip.h"
#include "IfxStm.h"
#include "UART_Logging.h"
#include <string.h>

/*******************************************************************************
 * Client State Variables
 ******************************************************************************/

static DoIP_ClientConfig    g_config;
static struct tcp_pcb      *g_pcb = NULL;
static DoIP_ClientState     g_state = DOIP_STATE_IDLE;

/* Timing */
static uint32 g_connect_start_time = 0;
static uint32 g_routing_request_time = 0;
static uint32 g_last_reconnect_attempt = 0;
static uint32 g_connection_ready_time = 0;

/* Receive buffer */
static uint8  g_rx_buffer[DOIP_RX_BUFFER_SIZE];
static uint16 g_rx_length = 0;

/* Flags for async events */
static volatile boolean g_connected_flag = FALSE;
static volatile boolean g_error_flag = FALSE;
static volatile boolean g_send_routing_activation = FALSE;

/*******************************************************************************
 * Helper Functions
 ******************************************************************************/

static uint32 GetTimestamp(void)
{
    Ifx_TickTime ticks;
    ticks = IfxStm_get(&MODULE_STM0);
    return (uint32)ticks;
}

static uint32 GetElapsedMs(uint32 start_time)
{
    uint32 now = GetTimestamp();
    Ifx_TickTime ticks_per_ms = IfxStm_getTicksFromMilliseconds(&MODULE_STM0, 1);
    return (now - start_time) / (uint32)ticks_per_ms;
}

static void SetState(DoIP_ClientState new_state)
{
    g_state = new_state;
}

/*******************************************************************************
 * Forward Declarations
 ******************************************************************************/

static void ProcessReceivedMessages(void);

/*******************************************************************************
 * lwIP Callback Functions
 ******************************************************************************/

static err_t doip_connected_callback(void *arg, struct tcp_pcb *tpcb, err_t err)
{
    (void)arg;
    (void)tpcb;
    
    if (err == ERR_OK)
    {
        /* Set flag - actual processing in Poll */
        g_connected_flag = TRUE;
    }
    else
    {
        g_error_flag = TRUE;
    }
    
    return ERR_OK;
}

static void doip_error_callback(void *arg, err_t err)
{
    (void)arg;
    (void)err;
    
    /* Connection error - set flag */
    g_error_flag = TRUE;
    g_pcb = NULL;  /* lwIP already freed the PCB */
}

static err_t doip_recv_callback(void *arg, struct tcp_pcb *tpcb, struct pbuf *p, err_t err)
{
    (void)arg;
    
    if (p == NULL)
    {
        /* Connection closed by remote */
        sendUARTMessage("[DoIP] Connection closed by VMG\r\n", 35);
        tcp_close(tpcb);
        g_pcb = NULL;
        g_error_flag = TRUE;
        return ERR_OK;
    }
    
    if (err != ERR_OK)
    {
        pbuf_free(p);
        return err;
    }
    
    /* Copy data to receive buffer */
    uint16 copy_len = p->tot_len;
    if (copy_len > (DOIP_RX_BUFFER_SIZE - g_rx_length))
    {
        copy_len = DOIP_RX_BUFFER_SIZE - g_rx_length;
    }
    
    pbuf_copy_partial(p, &g_rx_buffer[g_rx_length], copy_len, 0);
    g_rx_length += copy_len;
    
    sendUARTMessage("[DoIP] RX: ", 11);
    char buf[20];
    uint8 len = 0;
    uint32 val = copy_len;
    do { buf[len++] = '0' + (val % 10); val /= 10; } while (val > 0);
    while (len > 0) { char c = buf[--len]; sendUARTMessage(&c, 1); }
    sendUARTMessage(" bytes\r\n", 8);
    
    /* Process received data immediately */
    ProcessReceivedMessages();
    
    /* Acknowledge received data */
    tcp_recved(tpcb, p->tot_len);
    pbuf_free(p);
    
    return ERR_OK;
}

/*******************************************************************************
 * Message Processing
 ******************************************************************************/

static void ProcessReceivedMessages(void)
{
    while (g_rx_length >= DOIP_HEADER_SIZE)
    {
        /* Parse header */
        DoIP_Header header;
        if (!DoIP_ParseHeader(g_rx_buffer, &header))
        {
            /* Invalid header - discard 1 byte and retry */
            sendUARTMessage("[DoIP] Invalid header\r\n", 23);
            memmove(g_rx_buffer, &g_rx_buffer[1], g_rx_length - 1);
            g_rx_length--;
            continue;
        }
        
        /* Check if we have complete message */
        uint32 total_len = DOIP_HEADER_SIZE + header.payloadLength;
        if (g_rx_length < total_len)
        {
            /* Wait for more data */
            break;
        }
        
        /* Process message based on type */
        const uint8 *payload = &g_rx_buffer[DOIP_HEADER_SIZE];
        
        if (header.payloadType == DOIP_ROUTING_ACTIVATION_RES)
        {
            sendUARTMessage("[DoIP] RX: Routing Activation Response\r\n", 41);
            uint8 response_code;
            if (DoIP_ParseRoutingActivationResponse(payload, header.payloadLength, &response_code))
            {
                if (response_code == DOIP_RA_RES_SUCCESS)
                {
                    SetState(DOIP_STATE_ACTIVE);
                    sendUARTMessage("[DoIP] Routing Activation SUCCESS\r\n", 37);
                }
                else
                {
                    sendUARTMessage("[DoIP] Routing Activation FAILED\r\n", 36);
                    g_error_flag = TRUE;
                }
            }
            else
            {
                sendUARTMessage("[DoIP] Parse error\r\n", 20);
            }
        }
        else if (header.payloadType == DOIP_ALIVE_CHECK_REQ)
        {
            sendUARTMessage("[DoIP] RX: Alive Check Request\r\n", 34);
            /* Send Alive Check Response */
            uint8 response_buffer[DOIP_HEADER_SIZE + 2];
            uint16 len = DoIP_CreateAliveCheckResponse(response_buffer, g_config.source_address);
            tcp_write(g_pcb, response_buffer, len, TCP_WRITE_FLAG_COPY);
            sendUARTMessage("[DoIP] TX: Alive Check Response\r\n", 35);
        }
        else if (header.payloadType == DOIP_DIAGNOSTIC_MESSAGE)
        {
            sendUARTMessage("[DoIP] RX: Diagnostic Message\r\n", 33);
            
            /* Parse UDS request from DoIP payload */
            UDS_Request uds_request;
            if (UDS_ParseDoIPDiagnostic(payload, header.payloadLength, &uds_request))
            {
                /* Handle UDS request and generate response */
                UDS_Response uds_response;
                if (UDS_HandleRequest(&uds_request, &uds_response))
                {
                    /* Build DoIP diagnostic message with UDS response */
                    uint8 response_buffer[DOIP_RX_BUFFER_SIZE];
                    uint16 response_len = UDS_BuildDoIPDiagnostic(&uds_response, response_buffer, sizeof(response_buffer));
                    
                    if (response_len > 0 && g_pcb != NULL)
                    {
                        /* Send response */
                        err_t err = tcp_write(g_pcb, response_buffer, response_len, TCP_WRITE_FLAG_COPY);
                        if (err == ERR_OK)
                        {
                            tcp_output(g_pcb);  /* Flush immediately */
                            sendUARTMessage("[DoIP] TX: Diagnostic Response sent\r\n", 39);
                        }
                        else
                        {
                            sendUARTMessage("[DoIP] TX: Failed to send response\r\n", 38);
                        }
                    }
                }
            }
        }
        
        /* Remove processed message from buffer */
        g_rx_length -= total_len;
        if (g_rx_length > 0)
        {
            memmove(g_rx_buffer, &g_rx_buffer[total_len], g_rx_length);
        }
    }
}

/*******************************************************************************
 * Connection Management
 ******************************************************************************/

static void DoIP_ConnectToVMG(void)
{
    if (g_pcb != NULL)
    {
        return;  /* Already have a PCB */
    }
    
    /* Create new TCP PCB */
    g_pcb = tcp_new();
    if (g_pcb == NULL)
    {
        SetState(DOIP_STATE_ERROR);
        return;
    }
    
    /* Set callbacks */
    tcp_err(g_pcb, doip_error_callback);
    tcp_recv(g_pcb, doip_recv_callback);
    
    /* Initiate connection */
    err_t err = tcp_connect(g_pcb, &g_config.vmg_ip, g_config.vmg_port, doip_connected_callback);
    
    if (err == ERR_OK)
    {
        SetState(DOIP_STATE_CONNECTING);
        g_connect_start_time = GetTimestamp();
        g_connected_flag = FALSE;
        g_error_flag = FALSE;
    }
    else
    {
        tcp_abort(g_pcb);
        g_pcb = NULL;
        SetState(DOIP_STATE_ERROR);
    }
}

static void DoIP_Cleanup(void)
{
    if (g_pcb != NULL)
    {
        tcp_abort(g_pcb);
        g_pcb = NULL;
    }
    
    g_rx_length = 0;
    g_connected_flag = FALSE;
    g_error_flag = FALSE;
    g_send_routing_activation = FALSE;
    SetState(DOIP_STATE_IDLE);
}

/*******************************************************************************
 * Public Functions
 ******************************************************************************/

void DoIP_Client_Init(const DoIP_ClientConfig *config)
{
    /* Copy configuration */
    g_config = *config;
    
    /* Initialize state */
    g_state = DOIP_STATE_IDLE;
    g_pcb = NULL;
    g_rx_length = 0;
    g_connected_flag = FALSE;
    g_error_flag = FALSE;
    g_send_routing_activation = FALSE;
    
    sendUARTMessage("[DoIP] Client initialized\r\n", 29);
}

void DoIP_Client_Poll(void)
{
    uint32 now = GetTimestamp();
    
    /* Handle async connection event */
    if (g_connected_flag)
    {
        g_connected_flag = FALSE;
        SetState(DOIP_STATE_CONNECTED);
        g_connection_ready_time = now;
        g_send_routing_activation = TRUE;
        sendUARTMessage("[DoIP] TCP connected\r\n", 24);
    }
    
    /* Handle async error event */
    if (g_error_flag)
    {
        g_error_flag = FALSE;
        sendUARTMessage("[DoIP] Connection error\r\n", 27);
        DoIP_Cleanup();
        g_last_reconnect_attempt = now;
        return;
    }
    
    /* State machine */
    switch (g_state)
    {
        case DOIP_STATE_IDLE:
        {
            /* Try to connect after reconnect interval */
            if (GetElapsedMs(g_last_reconnect_attempt) >= DOIP_RECONNECT_INTERVAL)
            {
                DoIP_ConnectToVMG();
                g_last_reconnect_attempt = now;
            }
            break;
        }
        
        case DOIP_STATE_CONNECTING:
        {
            /* Check connection timeout */
            if (GetElapsedMs(g_connect_start_time) >= DOIP_TIMEOUT_CONNECTION)
            {
                sendUARTMessage("[DoIP] Connection timeout\r\n", 29);
                DoIP_Cleanup();
                g_last_reconnect_attempt = now;
            }
            break;
        }
        
        case DOIP_STATE_CONNECTED:
        {
            /* Wait 200ms for TCP buffers to be ready */
            if (g_send_routing_activation && 
                GetElapsedMs(g_connection_ready_time) >= 200)
            {
                g_send_routing_activation = FALSE;
                
                /* Send Routing Activation Request */
                uint8 request_buffer[DOIP_HEADER_SIZE + 7];
                uint16 len = DoIP_CreateRoutingActivationRequest(request_buffer, g_config.source_address);
                
                if (tcp_write(g_pcb, request_buffer, len, TCP_WRITE_FLAG_COPY) == ERR_OK)
                {
                    g_routing_request_time = now;
                    sendUARTMessage("[DoIP] Routing Activation Request sent\r\n", 43);
                }
                else
                {
                    g_error_flag = TRUE;
                }
            }
            
            /* Check routing activation timeout */
            if (!g_send_routing_activation && 
                GetElapsedMs(g_routing_request_time) >= DOIP_TIMEOUT_ROUTING)
            {
                sendUARTMessage("[DoIP] Routing timeout\r\n", 26);
                DoIP_Cleanup();
                g_last_reconnect_attempt = now;
            }
            
            break;
        }
        
        case DOIP_STATE_ACTIVE:
        {
            /* Process received messages */
            ProcessReceivedMessages();
            break;
        }
        
        case DOIP_STATE_ERROR:
        {
            DoIP_Cleanup();
            g_last_reconnect_attempt = now;
            break;
        }
    }
}

DoIP_ClientState DoIP_Client_GetState(void)
{
    return g_state;
}

boolean DoIP_Client_IsActive(void)
{
    return (g_state == DOIP_STATE_ACTIVE);
}

boolean DoIP_Client_SendHealthStatusReport(uint8 ecu_count, const DoIP_HealthStatus_Info *health_data)
{
    if (g_state != DOIP_STATE_ACTIVE || g_pcb == NULL)
    {
        return FALSE;
    }
    
    if (ecu_count == 0 || health_data == NULL)
    {
        return FALSE;
    }
    
    /* Create Health Status Report message */
    uint8 buffer[DOIP_HEADER_SIZE + 1 + (sizeof(DoIP_HealthStatus_Info) * MAX_ZONE_ECUS)];
    
    /* DoIP Header */
    buffer[0] = DOIP_PROTOCOL_VERSION;
    buffer[1] = DOIP_INVERSE_VERSION;
    buffer[2] = (DOIP_HEALTH_STATUS_REPORT >> 8) & 0xFF;
    buffer[3] = DOIP_HEALTH_STATUS_REPORT & 0xFF;
    
    /* Payload length = 1 byte (count) + (size per ECU * ecu_count) */
    uint32 payload_len = 1 + (sizeof(DoIP_HealthStatus_Info) * ecu_count);
    buffer[4] = (payload_len >> 24) & 0xFF;
    buffer[5] = (payload_len >> 16) & 0xFF;
    buffer[6] = (payload_len >> 8) & 0xFF;
    buffer[7] = payload_len & 0xFF;
    
    /* ECU Count */
    buffer[8] = ecu_count;
    
    /* Copy Health data for each ECU */
    uint16 offset = 9;
    for (uint8 i = 0; i < ecu_count; i++)
    {
        memcpy(&buffer[offset], &health_data[i], sizeof(DoIP_HealthStatus_Info));
        offset += sizeof(DoIP_HealthStatus_Info);
    }
    
    /* Send */
    uint16 total_len = DOIP_HEADER_SIZE + payload_len;
    err_t err = tcp_write(g_pcb, buffer, total_len, TCP_WRITE_FLAG_COPY);
    
    if (err == ERR_OK)
    {
        sendUARTMessage("[Health] Status report sent (", 29);
        char count_str[4];
        count_str[0] = '0' + ecu_count;
        sendUARTMessage(count_str, 1);
        sendUARTMessage(" ECUs)\r\n", 8);
        return TRUE;
    }
    
    return FALSE;
}

boolean DoIP_Client_SendVCIReport(uint8 vci_count, const DoIP_VCI_Info *vci_database)
{
    if (g_state != DOIP_STATE_ACTIVE || g_pcb == NULL)
    {
        return FALSE;
    }
    
    if (vci_count == 0 || vci_database == NULL)
    {
        return FALSE;
    }
    
    /* Create VCI Report message */
    uint8 buffer[DOIP_HEADER_SIZE + 1 + (sizeof(DoIP_VCI_Info) * MAX_ZONE_ECUS)];
    
    /* DoIP Header */
    buffer[0] = DOIP_PROTOCOL_VERSION;
    buffer[1] = DOIP_INVERSE_VERSION;
    buffer[2] = (DOIP_VCI_REPORT >> 8) & 0xFF;
    buffer[3] = DOIP_VCI_REPORT & 0xFF;
    
    /* Payload length = 1 byte (count) + (48 bytes per ECU * vci_count) */
    uint32 payload_len = 1 + (sizeof(DoIP_VCI_Info) * vci_count);
    buffer[4] = (payload_len >> 24) & 0xFF;
    buffer[5] = (payload_len >> 16) & 0xFF;
    buffer[6] = (payload_len >> 8) & 0xFF;
    buffer[7] = payload_len & 0xFF;
    
    /* VCI Count */
    buffer[8] = vci_count;
    
    /* Copy VCI data for each ECU */
    uint16 offset = 9;
    for (uint8 i = 0; i < vci_count; i++)
    {
        memcpy(&buffer[offset], &vci_database[i], sizeof(DoIP_VCI_Info));
        offset += sizeof(DoIP_VCI_Info);
    }
    
    /* Send */
    uint16 total_len = DOIP_HEADER_SIZE + payload_len;
    err_t err = tcp_write(g_pcb, buffer, total_len, TCP_WRITE_FLAG_COPY);
    
    if (err == ERR_OK)
    {
        sendUARTMessage("[VCI] Report sent to VMG (", 26);
        char count_str[4];
        count_str[0] = '0' + vci_count;
        count_str[1] = ' ';
        count_str[2] = 'E';
        count_str[3] = 'C';
        sendUARTMessage(count_str, 1);
        sendUARTMessage(" ECUs)\r\n", 8);
        return TRUE;
    }
    
    return FALSE;
}

void DoIP_Client_Close(void)
{
    DoIP_Cleanup();
}

/*******************************************************************************
 * UDS-based VCI Request Functions
 ******************************************************************************/

boolean DoIP_Client_RequestConsolidatedVCI(void)
{
    if (g_state != DOIP_STATE_ACTIVE || g_pcb == NULL)
    {
        return FALSE;
    }
    
    /* Create UDS Request for Consolidated VCI (DID 0xF195) */
    UDS_Request request;
    request.source_address = ZGW_ADDRESS;     /* 0x0100 - Zonal Gateway */
    request.target_address = VMG_ADDRESS;     /* 0x0200 - VMG */
    request.service_id = UDS_SID_READ_DATA_BY_IDENTIFIER;  /* 0x22 */
    request.data_len = 2;
    request.data[0] = (UDS_DID_VCI_CONSOLIDATED >> 8) & 0xFF;  /* DID High */
    request.data[1] = UDS_DID_VCI_CONSOLIDATED & 0xFF;         /* DID Low (0xF195) */
    
    /* Build DoIP Diagnostic Message */
    uint8 buffer[256];
    UDS_Response dummy_response;  /* Not used for requests */
    dummy_response.source_address = request.source_address;
    dummy_response.target_address = request.target_address;
    dummy_response.is_positive = TRUE;
    dummy_response.service_id = request.service_id;
    dummy_response.data_len = request.data_len;
    memcpy(dummy_response.data, request.data, request.data_len);
    
    uint16 msg_len = UDS_BuildDoIPDiagnostic(&dummy_response, buffer, sizeof(buffer));
    
    if (msg_len == 0)
    {
        return FALSE;
    }
    
    /* Send via TCP */
    err_t err = tcp_write(g_pcb, buffer, msg_len, TCP_WRITE_FLAG_COPY);
    
    if (err == ERR_OK)
    {
        sendUARTMessage("[UDS] VCI Request sent (DID 0xF195)\r\n", 38);
        return TRUE;
    }
    
    return FALSE;
}

boolean DoIP_Client_RequestHealthStatus(void)
{
    if (g_state != DOIP_STATE_ACTIVE || g_pcb == NULL)
    {
        return FALSE;
    }
    
    /* Create UDS Request for Health Status (DID 0xF1A0) */
    UDS_Request request;
    request.source_address = ZGW_ADDRESS;
    request.target_address = VMG_ADDRESS;
    request.service_id = UDS_SID_READ_DATA_BY_IDENTIFIER;
    request.data_len = 2;
    request.data[0] = (UDS_DID_HEALTH_STATUS >> 8) & 0xFF;
    request.data[1] = UDS_DID_HEALTH_STATUS & 0xFF;
    
    /* Build DoIP Diagnostic Message */
    uint8 buffer[256];
    UDS_Response dummy_response;
    dummy_response.source_address = request.source_address;
    dummy_response.target_address = request.target_address;
    dummy_response.is_positive = TRUE;
    dummy_response.service_id = request.service_id;
    dummy_response.data_len = request.data_len;
    memcpy(dummy_response.data, request.data, request.data_len);
    
    uint16 msg_len = UDS_BuildDoIPDiagnostic(&dummy_response, buffer, sizeof(buffer));
    
    if (msg_len == 0)
    {
        return FALSE;
    }
    
    /* Send via TCP */
    err_t err = tcp_write(g_pcb, buffer, msg_len, TCP_WRITE_FLAG_COPY);
    
    if (err == ERR_OK)
    {
        sendUARTMessage("[UDS] Health Request sent (DID 0xF1A0)\r\n", 41);
        return TRUE;
    }
    
    return FALSE;
}

