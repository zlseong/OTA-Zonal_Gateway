/**
 * @file SystemMain.c
 * @brief System Main Loop Manager Implementation
 */

#include "SystemMain.h"
#include "Ifx_Lwip.h"
#include "Libraries/DoIP/doip_client.h"
#include "Libraries/DataCollection/vci_manager.h"
#include "Libraries/DataCollection/readiness_manager.h"

void SystemMain_Loop(void)
{
    while (1)
    {
        Ifx_Lwip_pollTimerFlags();
        Ifx_Lwip_pollReceiveFlags();
        DoIP_Client_Poll();
        VCI_CheckCollectionTimeout();
        Readiness_CheckTimeout();
    }
}

