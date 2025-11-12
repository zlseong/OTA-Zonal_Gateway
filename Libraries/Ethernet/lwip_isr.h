/**********************************************************************************************************************
 * \file lwip_isr.h
 * \copyright Copyright (C) Infineon Technologies AG 2019
 * 
 * lwIP Timer Interrupt Service Routine - Interface
 *********************************************************************************************************************/

#ifndef LWIP_ISR_H_
#define LWIP_ISR_H_

#include "Ifx_Types.h"

/* ISR Declaration (called from vector table) */
void updateLwIPStackISR(void);

#endif /* LWIP_ISR_H_ */

