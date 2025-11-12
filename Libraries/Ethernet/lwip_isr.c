/**********************************************************************************************************************
 * \file lwip_isr.c
 * \copyright Copyright (C) Infineon Technologies AG 2019
 * 
 * lwIP Timer Interrupt Service Routine Implementation
 *********************************************************************************************************************/

#include "lwip_isr.h"
#include "Configuration.h"
#include "IfxStm.h"
#include "Ifx_Lwip.h"

extern volatile uint32 g_TickCount_1ms;

/**
 * @brief lwIP Timer ISR (1ms period)
 * 
 * This ISR is called every 1ms by STM0 Compare 0
 * Updates lwIP stack timers for ARP, TCP, DHCP, etc.
 */
IFX_INTERRUPT(updateLwIPStackISR, 0, ISR_PRIORITY_OS_TICK);
void updateLwIPStackISR(void)
{
    /* Configure STM to generate next interrupt in 1ms */
    IfxStm_increaseCompare(&MODULE_STM0, IfxStm_Comparator_0, IFX_CFG_STM_TICKS_PER_MS);
    
    /* Increase lwIP system time */
    g_TickCount_1ms++;
    
    /* Update lwIP timers for all enabled protocols (ARP, TCP, DHCP, LINK) */
    Ifx_Lwip_onTimerTick();
}

