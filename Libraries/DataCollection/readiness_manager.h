/**********************************************************************************************************************
 * \file readiness_manager.h
 * \copyright Copyright (C) Infineon Technologies AG 2019
 * 
 * Readiness Collection Manager - Interface
 * 
 * This module manages OTA update readiness check across all ECUs in the zone.
 * It follows the same pattern as VCI collection (UDP broadcast).
 *********************************************************************************************************************/

#ifndef READINESS_MANAGER_H_
#define READINESS_MANAGER_H_

#include "Ifx_Types.h"

/*******************************************************************************
 * Readiness Information Structure
 ******************************************************************************/

/**
 * \brief Readiness Information for a single ECU
 */
typedef struct
{
    char ecu_id[16];                 /* ECU ID (e.g., "ECU_091") */
    
    /* Condition Information */
    boolean vehicle_parked;          /* TRUE if gear is P or N */
    boolean engine_off;              /* TRUE if engine/motor is off */
    uint16 battery_voltage_mv;       /* Battery voltage in mV (e.g., 12800 = 12.8V) */
    uint32 available_memory_kb;      /* Available flash memory in KB */
    boolean all_doors_closed;        /* TRUE if all doors are closed */
    
    /* Compatibility (based on VCI) */
    boolean compatible;              /* TRUE if SW is compatible */
    
    /* Final Decision */
    boolean ready_for_update;        /* TRUE if all conditions are met */
    
} Readiness_Info;

/*******************************************************************************
 * Configuration
 ******************************************************************************/

#define MAX_READINESS_ECUS              10      /* Maximum ECUs for readiness check */
#define READINESS_COLLECTION_TIMEOUT    5000    /* Readiness collection timeout: 5 seconds */

/*******************************************************************************
 * Function Prototypes
 ******************************************************************************/

/**
 * \brief Initialize Readiness Manager
 */
void Readiness_Init(void);

/**
 * \brief Start readiness check (UDP broadcast to all ECUs)
 */
void Readiness_StartCheck(void);

/**
 * \brief Check if readiness collection is complete
 * \return TRUE if collection is complete or timed out
 */
boolean Readiness_IsCheckComplete(void);

/**
 * \brief Check readiness collection timeout
 */
void Readiness_CheckTimeout(void);

/**
 * \brief Get consolidated readiness information
 * \param readiness_array Output array for readiness info
 * \param readiness_count Output count of ECUs
 * \return TRUE if successful
 */
boolean Readiness_GetConsolidated(Readiness_Info* readiness_array, uint8* readiness_count);

/**
 * \brief Get local ECU's readiness information
 * \return Local readiness info
 */
Readiness_Info Readiness_GetLocal(void);

/**
 * \brief Handle incoming readiness response from Zone ECU
 * \param readiness_info Readiness info from Zone ECU
 */
void Readiness_HandleResponse(const Readiness_Info* readiness_info);

#endif /* READINESS_MANAGER_H_ */

