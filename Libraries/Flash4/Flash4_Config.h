/**
 * @file Flash4_Config.h
 * @brief MIKROE-3191 Flash 4 Click Configuration
 */

#ifndef FLASH4_CONFIG_H_
#define FLASH4_CONFIG_H_

/* QSPI Module Selection */
#define FLASH4_QSPI_MODULE              &MODULE_QSPI2

/* Baudrate Configuration */
#define FLASH4_QSPI_MAX_BAUDRATE        100000000UL
#define FLASH4_QSPI_BAUDRATE            25000000.0f
#define FLASH4_BAUDRATE                 25000000

/* Interrupt Priorities (0-255, lower = higher priority) */
#define ISR_PRIORITY_FLASH4_TX          10
#define ISR_PRIORITY_FLASH4_RX          11
#define ISR_PRIORITY_FLASH4_ER          12

/* Interrupt priority defines for ISR macros */
#define IFX_INTPRIO_QSPI2_TX            ISR_PRIORITY_FLASH4_TX
#define IFX_INTPRIO_QSPI2_RX            ISR_PRIORITY_FLASH4_RX
#define IFX_INTPRIO_QSPI2_ER            ISR_PRIORITY_FLASH4_ER

#endif /* FLASH4_CONFIG_H_ */

