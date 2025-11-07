# OTA Package Manager

## Overview

Dynamic OTA firmware package management system for 64MB SPI Flash.

---

## Features

- ✅ **Dynamic Memory Allocation** - No fixed slots, efficient space usage
- ✅ **Package Metadata Table** - Up to 32 packages simultaneously
- ✅ **Garbage Collection** - Automatic cleanup of deployed packages
- ✅ **Multiple Target Types** - ZGW, CAN ECU, LIN ECU, ETH ECU
- ✅ **Deployment Conditions** - Immediate, Ignition Off, Vehicle Stop, Manual
- ✅ **Persistent Metadata** - Survives power cycles

---

## Memory Layout

```
SPI Flash (64MB):
┌────────────────────────────────────┐
│ 0x000000 - 0x000FFF (4KB)          │
│ [OTA Metadata]                     │
│ - Magic: 0x4F54414D ("OTAM")       │
│ - Package Table (32 entries)      │
│ - Free Space Pointers              │
├────────────────────────────────────┤
│ 0x001000 - 0x3FFFFFF (63.996MB)    │
│ [Dynamic Package Storage]          │
│ - Package #0 (ZGW: 512KB)          │
│ - Package #1 (ECU_011: 256KB)      │
│ - Package #2 (ECU_025: 64KB)       │
│ - ...                              │
│ - [Free Space]                     │
└────────────────────────────────────┘
```

---

## API Usage

### Initialize

```c
#include "ota_manager.h"

OTA_Manager ota;
SPI_Flash_S25FL512S flash;

// Initialize Flash first
SPI_Flash_Init(&flash);

// Initialize OTA Manager
OTA_Result result = OTA_Manager_Init(&ota, &flash);
if (result == OTA_OK) {
    printf("OTA Manager Ready\n");
}
```

### Allocate Space for New Package

```c
uint32 package_size = 524288;  /* 512KB */
uint32 flash_address = 0;

result = OTA_Manager_AllocatePackage(&ota, package_size, &flash_address);
if (result == OTA_OK) {
    printf("Allocated at 0x%06X\n", flash_address);
    
    // Write package data to flash_address
    // ...
}
```

### Add Package Entry

```c
result = OTA_Manager_AddPackage(&ota,
                                 "ZGW_001",           // Target ECU ID
                                 OTA_TARGET_ZGW,      // Target Type
                                 OTA_BUS_INTERNAL,    // Bus Type
                                 OTA_CONDITION_IGN_OFF, // Condition
                                 flash_address,       // Flash Address
                                 package_size,        // Size
                                 0x12345678);         // CRC32

if (result == OTA_OK) {
    printf("Package added\n");
}
```

### Find Package

```c
OTA_PackageEntry entry;
result = OTA_Manager_FindPackage(&ota, "ECU_011", &entry);

if (result == OTA_OK) {
    printf("Found at 0x%06X, Size: %u bytes\n",
           entry.flash_address, entry.package_size);
}
```

### Update Status

```c
result = OTA_Manager_UpdatePackageStatus(&ota, "ECU_011", OTA_STATUS_DEPLOYING);
// ... deploy ...
result = OTA_Manager_UpdatePackageStatus(&ota, "ECU_011", OTA_STATUS_DEPLOYED);
```

### Garbage Collection

```c
result = OTA_Manager_GarbageCollect(&ota);
// Removes all packages with status OTA_STATUS_DEPLOYED
```

### Get Statistics

```c
uint16 package_count;
uint32 free_space, total_received;

OTA_Manager_GetStatistics(&ota, &package_count, &free_space, &total_received);

printf("Packages: %u, Free: %u KB, Total Received: %u\n",
       package_count, free_space / 1024, total_received);
```

---

## Package Status Flow

```
EMPTY
  ↓ (Allocate)
DOWNLOADING
  ↓ (Complete)
COMPLETE
  ↓ (Ready to deploy)
PENDING
  ↓ (Start deployment)
DEPLOYING
  ↓ (Success)
DEPLOYED
  ↓ (Garbage Collection)
EMPTY
```

---

## Example Scenario

### Scenario: Update 3 ECUs (ZGW + 2 Zone ECUs)

```c
// 1. Receive ZGW update (512KB)
uint32 zgw_addr;
OTA_Manager_AllocatePackage(&ota, 512*1024, &zgw_addr);
// ... download to zgw_addr ...
OTA_Manager_AddPackage(&ota, "ZGW_001", OTA_TARGET_ZGW, ...);

// 2. Receive ECU_011 update (256KB)
uint32 ecu11_addr;
OTA_Manager_AllocatePackage(&ota, 256*1024, &ecu11_addr);
// ... download to ecu11_addr ...
OTA_Manager_AddPackage(&ota, "ECU_011", OTA_TARGET_CAN_ECU, ...);

// 3. Receive ECU_025 update (64KB)
uint32 ecu25_addr;
OTA_Manager_AllocatePackage(&ota, 64*1024, &ecu25_addr);
// ... download to ecu25_addr ...
OTA_Manager_AddPackage(&ota, "ECU_025", OTA_TARGET_CAN_ECU, ...);

// Total Used: 832KB
// Free Space: ~63.2MB
```

---

## Memory Efficiency Comparison

| Approach | ZGW (512KB) | ECU_011 (256KB) | ECU_025 (64KB) | **Total** |
|----------|-------------|-----------------|----------------|-----------|
| **Fixed 1MB Slots** | 1MB | 1MB | 1MB | **3MB** (2.2MB wasted) |
| **Dynamic Allocation** | 512KB | 256KB | 64KB | **832KB** (73% saved!) |

---

## Integration with UDS

See `UDS_TEST_GUIDE.md` for UDS integration:
- `0x34` RequestDownload
- `0x36` TransferData
- `0x37` RequestTransferExit

The OTA Manager integrates seamlessly with UDS Stage1 download.


