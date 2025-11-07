# S25FL512S SPI Flash Driver

## Overview

Driver for **Cypress/Infineon S25FL512S** 512Mbit (64MB) SPI Flash memory.

**Hardware:** MIKROE-3191 Flash Click (mikroBUS)

---

## Hardware Connection

### QSPI2 Pins (mikroBUS on TC375 Lite Kit)

| mikroBUS Pin | Function | TC375 Pin | QSPI2 Signal | Description |
|--------------|----------|-----------|--------------|-------------|
| 2            | **RST**  | P10.6     | GPIO         | Hardware Reset |
| 3            | **CS**   | P14.7     | SLSO4        | Chip Select |
| 4            | **SCK**  | P15.8     | SCLK         | Clock       |
| 5            | **SDO**  | P15.7     | MRSTB        | Master In (MISO) |
| 6            | **SDI**  | P15.6     | MTSR         | Master Out (MOSI) |

> **Note:** Pin mapping is based on the official TC375 Lite Kit mikroBUS connector schematic.

---

## ⚠️ Hardware Connection Test (MUST DO FIRST!)

**Before using the QSPI driver**, run the hardware test to verify connections:

### Test Files:
- `spi_flash_hardware_test.c` - GPIO bit-bang SPI test
- `spi_flash_hardware_test.h` - Test API

### Usage:
```c
#include "Libraries/SPI_Flash/spi_flash_hardware_test.h"

/* In your main() before any QSPI initialization */
SPI_Flash_HW_Test_Run_All();
```

### What it tests:
1. **GPIO Toggle Test**: CS, SCK, MOSI pins can output signals
2. **MISO Input Test**: MISO pin can read different states (pull-up/pull-down)
3. **Bit-bang SPI Test**: Read JEDEC ID without QSPI driver (pure GPIO)

### Expected output (SUCCESS):
```
[HW Test] Bit-bang RX: 9F 01 02 20
[HW Test]   -> Manufacturer: 0x01
[HW Test]   -> Memory Type:  0x02
[HW Test]   -> Capacity:     0x20
[HW Test] ✓ SUCCESS: S25FL512S detected!
```

### Troubleshooting:
- **All 0x00**: Flash IC not responding
  - Check: Power LED on Flash Click board
  - Check: mikroBUS orientation (notch alignment)
- **All 0xFF**: MISO pin not connected
  - Check: Pin P15.7 mapping in schematic
- **MISO test fail**: GPIO pin configuration error

---

## Features

- ✅ Read JEDEC ID
- ✅ Read/Write data
- ✅ 4KB Sector Erase
- ✅ 64KB Block Erase
- ✅ Chip Erase
- ✅ Status Register monitoring
- ✅ Page Program (256 bytes)
- ✅ Automatic page boundary handling

---

## API Usage

### Initialize

```c
#include "spi_flash_s25fl512s.h"

SPI_Flash_S25FL512S flash;
SPI_Flash_Result result = SPI_Flash_Init(&flash);

if (result == FLASH_OK) {
    printf("Flash OK: ID = 0x%02X%02X%02X\n",
           flash.manufacturer_id,
           flash.memory_type,
           flash.capacity);
}
```

### Read Data

```c
uint8 buffer[256];
result = SPI_Flash_Read(&flash, 0x001000, buffer, 256);
```

### Write Data

```c
uint8 data[64] = { /* ... */ };
result = SPI_Flash_Write(&flash, 0x001000, data, 64);
```

### Erase Sector

```c
result = SPI_Flash_EraseSector(&flash, 0x001000);  /* 4KB */
```

---

## Memory Map

```
0x000000 - 0x3FFFFFF  (64MB total)
├─ 0x000000           (Reserved for OTA Manager Metadata)
└─ 0x001000           (OTA Package Storage)
```

---

## Timing

| Operation | Typical | Max |
|-----------|---------|-----|
| Page Program | 1.4ms | 3ms |
| 4KB Sector Erase | 180ms | 400ms |
| 64KB Block Erase | 1.2s | 3s |
| Chip Erase | 200s | 400s |

---

## Notes

- Always call `SPI_Flash_WaitReady()` after erase/program
- Write operations require `SPI_Flash_WriteEnable()` (called internally)
- Page writes crossing 256-byte boundary are automatically handled

