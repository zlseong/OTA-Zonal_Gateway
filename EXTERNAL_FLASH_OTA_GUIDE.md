# External Flash & OTA System Guide

## 🎯 **Overview**

Complete OTA (Over-The-Air) update system using external SPI Flash for Zonal Gateway.

---

## 📦 **Hardware**

### **Flash Memory**
- **Product**: MIKROE-3191 (Flash Click)
- **IC**: S25FL512S (Cypress/Infineon)
- **Capacity**: 512Mbit = **64MB**
- **Interface**: SPI / Quad SPI
- **Connector**: mikroBUS™

### **TC375 QSPI3 Connection**

| Function | TC375 Pin | Signal | Flash Click |
|----------|-----------|--------|-------------|
| **CS**   | P02.8     | SLSO8  | CS          |
| **SCLK** | P02.7     | SCLK   | SCK         |
| **MOSI** | P02.6     | MTSR   | SDI         |
| **MISO** | P02.5     | MRSTA  | SDO         |
| **3.3V** | -         | -      | VCC         |
| **GND**  | -         | -      | GND         |

---

## 🏗️ **System Architecture**

```
┌─────────────────────────────────────────────────────────┐
│  VMG (Vehicle Management Gateway)                       │
│  - Sends OTA packages via DoIP/UDS                      │
└───────────────────┬─────────────────────────────────────┘
                    │ Ethernet (DoIP)
                    │ UDS 0x34/0x36/0x37
                    ↓
┌─────────────────────────────────────────────────────────┐
│  Zonal Gateway (TC375)                                  │
│  ┌─────────────────────────────────────────────────┐    │
│  │  Stage 1: UDS Download Handler                  │    │
│  │  - Receive package via UDS                      │    │
│  │  - Parse metadata (target, condition)           │    │
│  │  - Store in external SPI Flash                  │    │
│  └──────────────────┬──────────────────────────────┘    │
│                     │                                    │
│  ┌─────────────────────────────────────────────────┐    │
│  │  SPI Flash Driver (S25FL512S)                   │    │
│  │  - QSPI3 communication                          │    │
│  │  - Read/Write/Erase operations                  │    │
│  └──────────────────┬──────────────────────────────┘    │
│                     │                                    │
│  ┌─────────────────────────────────────────────────┐    │
│  │  OTA Manager                                    │    │
│  │  - Dynamic memory allocation                    │    │
│  │  - Package metadata table                       │    │
│  │  - Garbage collection                           │    │
│  └──────────────────┬──────────────────────────────┘    │
│                     │                                    │
│  ┌─────────────────────────────────────────────────┐    │
│  │  External SPI Flash (64MB)                      │    │
│  │  0x000000: Metadata (4KB)                       │    │
│  │  0x001000: Package Storage (63.996MB)           │    │
│  │    - ZGW package                                │    │
│  │    - CAN ECU packages                           │    │
│  │    - LIN ECU packages                           │    │
│  └──────────────────┬──────────────────────────────┘    │
│                     │                                    │
│  ┌─────────────────────────────────────────────────┐    │
│  │  Stage 2: Deployment Manager                    │    │
│  │  - Read package from Flash                      │    │
│  │  - Check deployment condition                   │    │
│  │  - Deploy to target ECU                         │    │
│  │    • ZGW: Self-update (bootloader)              │    │
│  │    • CAN ECU: Transfer via CAN                  │    │
│  │    • LIN ECU: Transfer via LIN                  │    │
│  └─────────────────────────────────────────────────┘    │
│                     │                                    │
│                     ↓                                    │
│  ┌─────────────────────────────────────────────────┐    │
│  │  Target ECUs                                    │    │
│  │  - Receive firmware via CAN/LIN                 │    │
│  │  - Update and verify                            │    │
│  └─────────────────────────────────────────────────┘    │
└─────────────────────────────────────────────────────────┘
```

---

## 📂 **File Structure**

```
Zonal_Gateway/
├── Libraries/
│   ├── SPI_Flash/
│   │   ├── spi_flash_s25fl512s.h       ← Flash driver header
│   │   ├── spi_flash_s25fl512s.c       ← Flash driver implementation
│   │   └── README.md                   ← Flash driver guide
│   │
│   ├── OTA/
│   │   ├── ota_manager.h               ← OTA manager header
│   │   ├── ota_manager.c               ← OTA manager implementation
│   │   └── README.md                   ← OTA manager guide
│   │
│   └── DoIP/
│       ├── uds_handler.h               ← UDS handler (Stage 1)
│       └── uds_handler.c
│
├── Cpu0_Main.c                         ← Main application
├── EXTERNAL_FLASH_OTA_GUIDE.md         ← This file
└── UDS_TEST_GUIDE.md                   ← UDS testing guide
```

---

## 🔧 **Implementation Status**

### ✅ **Completed**

1. ✅ **SPI Flash Driver (S25FL512S)**
   - QSPI3 initialization
   - Read/Write/Erase operations
   - Read JEDEC ID
   - Status register monitoring
   - Comprehensive test suite

2. ✅ **OTA Manager**
   - Dynamic memory allocation
   - Package metadata table (32 packages)
   - Garbage collection
   - Persistent metadata storage

3. ✅ **Integration**
   - Flash driver integrated in Cpu0_Main.c
   - Test functions executed on startup
   - No build errors

### 🚧 **In Progress**

4. ⏳ **UDS Stage 1 Integration**
   - Integrate OTA Manager with UDS handler
   - Store downloaded packages in Flash
   - Parse package headers for routing

5. ⏳ **Stage 2 Deployment**
   - Implement deployment conditions
   - CAN/LIN transfer logic
   - Self-update (bootloader)

---

## 📋 **OTA Package Format**

### **Package Structure**

```
┌──────────────────────────────────────┐
│  Package Header (64 bytes)           │
│  ┌────────────────────────────────┐  │
│  │ Magic: 0x4F544150 ("OTAP")    │  │
│  │ Version: 0x0100                │  │
│  │ Target ECU ID: "ECU_011"       │  │
│  │ Target Type: CAN_ECU           │  │
│  │ Bus Type: CAN_FD               │  │
│  │ Condition: IGN_OFF             │  │
│  │ Firmware Size: 262144 (256KB)  │  │
│  │ Firmware CRC32: 0x12345678     │  │
│  │ Timestamp: ...                 │  │
│  └────────────────────────────────┘  │
├──────────────────────────────────────┤
│  Firmware Data (variable size)       │
│  - Binary firmware image             │
│  - Size: firmware_size bytes         │
└──────────────────────────────────────┘
```

### **Target Types**

| Code | Type | Description |
|------|------|-------------|
| 0x01 | ZGW | Zonal Gateway itself |
| 0x02 | CAN_ECU | CAN-connected ECU |
| 0x03 | LIN_ECU | LIN-connected ECU |
| 0x04 | ETH_ECU | Ethernet-connected ECU |

### **Deployment Conditions**

| Code | Condition | Description |
|------|-----------|-------------|
| 0x00 | IMMEDIATE | Deploy immediately |
| 0x01 | IGN_OFF | Deploy after ignition off |
| 0x02 | VEHICLE_STOP | Deploy when vehicle stopped |
| 0x03 | MANUAL | Deploy on manual trigger |

---

## 🔄 **OTA Workflow**

### **Stage 1: Download (UDS)**

```
VMG                    ZGW (TC375)            SPI Flash
 │                        │                        │
 │─── 0x10 01 ──────────→│ DiagnosticSession      │
 │                        │  (Programming)         │
 │                        │                        │
 │─── 0x34 ──────────────→│ RequestDownload        │
 │    (Header)            │ ┌──────────────────┐   │
 │                        │ │ Parse header:     │   │
 │                        │ │ - Target ECU      │   │
 │                        │ │ - Size, CRC       │   │
 │                        │ │ - Condition       │   │
 │                        │ └──────────────────┘   │
 │←─── 0x74 ──────────────│ Positive Response      │
 │                        │                        │
 │                        │ OTA_Manager_Allocate() │
 │                        │───────────────────────→│
 │                        │←───────────────────────│ Address
 │                        │                        │
 │─── 0x36 ──────────────→│ TransferData (Block 1) │
 │    (Firmware data)     │───────────────────────→│ Write
 │←─── 0x76 ──────────────│                        │
 │                        │                        │
 │─── 0x36 ──────────────→│ TransferData (Block 2) │
 │───────────────────────→│───────────────────────→│ Write
 │  ...                   │  ...                   │  ...
 │                        │                        │
 │─── 0x37 ──────────────→│ RequestTransferExit    │
 │                        │ OTA_Manager_AddPackage()│
 │                        │───────────────────────→│ Save Metadata
 │←─── 0x77 ──────────────│                        │
 │                        │                        │
 │─── 0x11 01 ───────────→│ ECUReset               │
 │                        │ (Optional)             │
```

### **Stage 2: Deployment**

```
ZGW (TC375)            SPI Flash              Target ECU
 │                        │                        │
 │ Check conditions       │                        │
 │ (IGN_OFF, etc.)        │                        │
 │                        │                        │
 │ Read package           │                        │
 │───────────────────────→│                        │
 │←───────────────────────│ Package data           │
 │                        │                        │
 │ Classify target:       │                        │
 │ ┌──────────────────┐   │                        │
 │ │ ZGW:     Bootloader│                        │
 │ │ CAN ECU: CAN FD    │                        │
 │ │ LIN ECU: LIN       │                        │
 │ └──────────────────┘   │                        │
 │                        │                        │
 │─── CAN/LIN Frames ────────────────────────────→│
 │                                                 │ Update
 │←─── ACK ───────────────────────────────────────│
 │                        │                        │
 │ Update status: DEPLOYED│                        │
 │───────────────────────→│                        │
```

---

## 🧪 **Testing**

### **1. Flash Test (Automatic on Boot)**

Run automatically in `Cpu0_Main.c`:

```
[SPI Flash] Testing S25FL512S (64MB)
===========================================
[Test 1] Reading JEDEC ID...
  Manufacturer: 0x01 (Expected: 0x01)
  Memory Type:  0x02 (Expected: 0x02)
  Capacity:     0x20 (Expected: 0x20 = 512Mbit)
[Test 2] Reading Status Register...
  Status: 0x00 (WIP=0, WEL=0)
[Test 3] Erasing 4KB sector at 0x001000...
  OK: Sector erased
[Test 4] Writing 64 bytes to 0x001000...
  OK: Data written
[Test 5] Reading 64 bytes from 0x001000...
  OK: Data read
[Test 6] Verifying data...
  OK: Data verified (100% match)
  First 16 bytes: A0 A1 A2 A3 A4 A5 A6 A7 A8 A9 AA AB AC AD AE AF
[Test 7] Testing 1KB write at 0x002000...
  OK: 1KB written
  OK: 1KB verified
===========================================
[SPI Flash] Test Complete!
===========================================
```

### **2. OTA Manager Test**

```c
OTA_Manager ota;
OTA_Manager_Init(&ota, &g_spi_flash);

// Get statistics
uint16 count;
uint32 free_space, total;
OTA_Manager_GetStatistics(&ota, &count, &free_space, &total);
printf("Packages: %u, Free: %u MB\n", count, free_space / 1024 / 1024);
```

### **3. UDS Download Test**

See `UDS_TEST_GUIDE.md` for complete UDS testing procedures.

---

## 📊 **Memory Efficiency**

### **Example: 3 ECU Updates**

| Package | Size | Fixed Slot (1MB) | Dynamic | Savings |
|---------|------|------------------|---------|---------|
| ZGW | 512KB | 1MB | 512KB | 512KB |
| ECU_011 | 256KB | 1MB | 256KB | 768KB |
| ECU_025 | 64KB | 1MB | 64KB | 960KB |
| **Total** | **832KB** | **3MB** | **832KB** | **2.2MB (73%)** |

---

## 🔐 **Security Features**

### **Implemented**
- ✅ CRC32 verification for firmware
- ✅ Package integrity checks
- ✅ Metadata validation (magic number)

### **Future Enhancements**
- 🔜 Digital signature verification
- 🔜 Encrypted firmware packages
- 🔜 Secure boot integration
- 🔜 Rollback protection

---

## 📈 **Performance**

### **Flash Operations**

| Operation | Size | Time (Typical) | Time (Max) |
|-----------|------|----------------|------------|
| Read | 1KB | < 1ms | < 2ms |
| Write (Page) | 256B | 1.4ms | 3ms |
| Erase Sector | 4KB | 180ms | 400ms |
| Erase Block | 64KB | 1.2s | 3s |

### **UDS Download Speed**

| Package Size | Estimated Time (@ 10 KB/s) |
|--------------|----------------------------|
| 64KB | 6.4 seconds |
| 256KB | 25.6 seconds |
| 512KB | 51.2 seconds |
| 1MB | 1 minute 42 seconds |

---

## 🚀 **Next Steps**

1. **UDS Integration** (Stage 1)
   - Connect UDS handler to OTA Manager
   - Test RequestDownload/TransferData/TransferExit

2. **Stage 2 Implementation**
   - Deployment condition monitoring
   - CAN/LIN firmware transfer
   - Bootloader integration for ZGW

3. **Testing & Validation**
   - End-to-end OTA workflow
   - Failure recovery scenarios
   - Long-term reliability testing

---

## 📚 **References**

- `Libraries/SPI_Flash/README.md` - Flash driver API
- `Libraries/OTA/README.md` - OTA Manager API
- `UDS_TEST_GUIDE.md` - UDS testing procedures
- S25FL512S Datasheet - Cypress/Infineon

---

**Status**: ✅ SPI Flash & OTA Manager Complete  
**Next**: 🚧 UDS Stage 1 Integration  
**Date**: 2025-11-05


