/**********************************************************************************************************************
 * \file vci_manager.h
 * \copyright Copyright (C) Infineon Technologies AG 2019
 * 
 * VCI Collection Manager - Interface
 *********************************************************************************************************************/

#ifndef VCI_MANAGER_H_
#define VCI_MANAGER_H_

#include "Ifx_Types.h"

/* Function Prototypes */
void VCI_SendCollectionRequest(void);
void VCI_StartCollection(void);
void VCI_CheckCollectionTimeout(void);

#endif /* VCI_MANAGER_H_ */

