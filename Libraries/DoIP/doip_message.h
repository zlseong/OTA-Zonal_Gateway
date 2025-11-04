/**
 * @file doip_message.h
 * @brief DoIP Message Serialization/Deserialization
 */

#ifndef DOIP_MESSAGE_H
#define DOIP_MESSAGE_H

#include "doip_types.h"

/*******************************************************************************
 * Function Prototypes
 ******************************************************************************/

/**
 * @brief Create DoIP header
 * @param buffer Output buffer (min 8 bytes)
 * @param payloadType Payload type
 * @param payloadLength Payload length
 */
void DoIP_CreateHeader(uint8 *buffer, uint16 payloadType, uint32 payloadLength);

/**
 * @brief Parse DoIP header
 * @param buffer Input buffer (min 8 bytes)
 * @param header Output header structure
 * @return TRUE if valid, FALSE otherwise
 */
boolean DoIP_ParseHeader(const uint8 *buffer, DoIP_Header *header);

/**
 * @brief Create Routing Activation Request
 * @param buffer Output buffer
 * @param sourceAddress Source logical address
 * @return Message length
 */
uint16 DoIP_CreateRoutingActivationRequest(uint8 *buffer, uint16 sourceAddress);

/**
 * @brief Parse Routing Activation Response
 * @param payload Payload data (without header)
 * @param payloadLength Payload length
 * @param responseCode Output response code
 * @return TRUE if successful, FALSE otherwise
 */
boolean DoIP_ParseRoutingActivationResponse(const uint8 *payload, uint32 payloadLength, uint8 *responseCode);

/**
 * @brief Create Alive Check Response
 * @param buffer Output buffer
 * @param sourceAddress Source logical address
 * @return Message length
 */
uint16 DoIP_CreateAliveCheckResponse(uint8 *buffer, uint16 sourceAddress);

/**
 * @brief Create Zone Status Report
 * @param buffer Output buffer
 * @param zoneCount Number of zones
 * @param zoneData Zone data array (5 bytes per zone: ID, Health, DTC, Battery, Temp)
 * @return Message length
 */
uint16 DoIP_CreateZoneStatusReport(uint8 *buffer, uint8 zoneCount, const uint8 *zoneData);

#endif /* DOIP_MESSAGE_H */

