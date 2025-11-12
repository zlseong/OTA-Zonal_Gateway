/**
 * @file doip_message.c
 * @brief DoIP Message Serialization/Deserialization Implementation
 */

#include "doip_message.h"
#include <string.h>

/*******************************************************************************
 * Helper Functions
 ******************************************************************************/

static void writeUint16BE(uint8 *buffer, uint16 value)
{
    buffer[0] = (uint8)((value >> 8) & 0xFF);
    buffer[1] = (uint8)(value & 0xFF);
}

static void writeUint32BE(uint8 *buffer, uint32 value)
{
    buffer[0] = (uint8)((value >> 24) & 0xFF);
    buffer[1] = (uint8)((value >> 16) & 0xFF);
    buffer[2] = (uint8)((value >> 8) & 0xFF);
    buffer[3] = (uint8)(value & 0xFF);
}

static uint16 readUint16BE(const uint8 *buffer)
{
    return ((uint16)buffer[0] << 8) | buffer[1];
}

static uint32 readUint32BE(const uint8 *buffer)
{
    return ((uint32)buffer[0] << 24) | ((uint32)buffer[1] << 16) |
           ((uint32)buffer[2] << 8) | buffer[3];
}

/*******************************************************************************
 * DoIP Message Functions
 ******************************************************************************/

void DoIP_CreateHeader(uint8 *buffer, uint16 payloadType, uint32 payloadLength)
{
    buffer[0] = DOIP_PROTOCOL_VERSION;
    buffer[1] = DOIP_INVERSE_VERSION;
    writeUint16BE(&buffer[2], payloadType);
    writeUint32BE(&buffer[4], payloadLength);
}

boolean DoIP_ParseHeader(const uint8 *buffer, DoIP_Header *header)
{
    /* Validate protocol version */
    if (buffer[0] != DOIP_PROTOCOL_VERSION || buffer[1] != DOIP_INVERSE_VERSION)
    {
        return FALSE;
    }
    
    header->protocolVersion = buffer[0];
    header->inverseProtocolVersion = buffer[1];
    header->payloadType = readUint16BE(&buffer[2]);
    header->payloadLength = readUint32BE(&buffer[4]);
    
    return TRUE;
}

uint16 DoIP_CreateRoutingActivationRequest(uint8 *buffer, uint16 sourceAddress)
{
    /* Create header */
    uint32 payloadLength = 7;  /* Source Address (2) + Activation Type (1) + Reserved (4) */
    DoIP_CreateHeader(buffer, DOIP_ROUTING_ACTIVATION_REQ, payloadLength);
    
    /* Create payload */
    writeUint16BE(&buffer[8], sourceAddress);   /* Source address */
    buffer[10] = 0x00;                          /* Activation type: Default */
    writeUint32BE(&buffer[11], 0x00000000);     /* Reserved */
    
    return DOIP_HEADER_SIZE + (uint16)payloadLength;
}

boolean DoIP_ParseRoutingActivationResponse(const uint8 *payload, uint32 payloadLength, uint8 *responseCode)
{
    /* Minimum payload: Source (2) + Target (2) + Response Code (1) + Reserved (4) = 9 bytes */
    if (payloadLength < 9)
    {
        return FALSE;
    }
    
    /* Extract response code (byte 4) */
    *responseCode = payload[4];
    
    return TRUE;
}

uint16 DoIP_CreateAliveCheckResponse(uint8 *buffer, uint16 sourceAddress)
{
    /* Create header */
    uint32 payloadLength = 2;  /* Source Address (2) */
    DoIP_CreateHeader(buffer, DOIP_ALIVE_CHECK_RES, payloadLength);
    
    /* Create payload */
    writeUint16BE(&buffer[8], sourceAddress);
    
    return DOIP_HEADER_SIZE + (uint16)payloadLength;
}

/* Legacy function - No longer used (replaced by DoIP_Client_SendHealthStatusReport) */
uint16 DoIP_CreateZoneStatusReport(uint8 *buffer, uint8 zoneCount, const uint8 *zoneData)
{
    /* Create header */
    uint32 payloadLength = 1 + (zoneCount * 5);  /* Zone Count (1) + (5 bytes per zone) */
    DoIP_CreateHeader(buffer, DOIP_HEALTH_STATUS_REPORT, payloadLength);
    
    /* Create payload */
    buffer[8] = zoneCount;
    
    /* Copy zone data */
    memcpy(&buffer[9], zoneData, zoneCount * 5);
    
    return DOIP_HEADER_SIZE + (uint16)payloadLength;
}

