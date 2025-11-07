# Software Package Routing & Dual Bank Flash - Implementation Guide

## 🎯 Overview

This implementation adds **intelligent software routing** and **dual-bank flash management** to the Zonal Gateway:

1. **Software Package Header**: 64-byte header with target ECU routing information
2. **ECU Classification**: Automatic detection of target ECU (ZGW vs Zone ECUs)
3. **Dual Bank Flash**: Safe OTA updates using A/B flash banks
4. **Routing Logic**: Forward Zone ECU packages to their destinations

---

## 📦 Software Package Format

### Complete Package Structure
```
[Header: 64 bytes] [Payload: N bytes]
```

### Header Fields (64 bytes total)

| Offset | Size | Field | Description |
|--------|------|-------|-------------|
| 0x00 | 4 | magic | 0x53575047 ("SWPG") |
| 0x04 | 2 | target_ecu_id | 0x0091=ZGW, 0x0011=Zone1, etc. |
| 0x06 | 1 | software_type | 0x01=APP, 0x02=BOOT, 0x03=CAL |
| 0x07 | 1 | compression | 0=none, 1=gzip |
| 0x08 | 4 | payload_size | Payload size in bytes |
| 0x0C | 4 | uncompressed_size | Original size if compressed |
| 0x10 | 1 | version_major | Major version |
| 0x11 | 1 | version_minor | Minor version |
| 0x12 | 1 | version_patch | Patch version |
| 0x13 | 1 | version_build | Build number |
| 0x14 | 4 | version_timestamp | Unix timestamp |
| 0x18 | 4 | version_serial | Serial number |
| 0x1C | 4 | crc32 | CRC32 of payload |
| 0x20 | 12 | signature | Reserved for digital signature |
| 0x2C | 2 | source_ecu_id | Source ECU (0x0E00=VMG) |
| 0x2E | 2 | hop_count | Hop count for multi-hop |
| 0x30 | 4 | sequence_number | Sequence number |
| 0x34 | 12 | reserved | Reserved for future use |

---

## 🏦 Dual Bank Flash Layout

### Flash Memory Map (16MB addressable)

| Address Range | Size | Purpose | Status |
|---------------|------|---------|--------|
| 0x00000000 - 0x000FFFFF | 1MB | Reserved (Boot/Config) | - |
| 0x00100000 - 0x001FFFFF | 1MB | Download Area (Temporary) | - |
| **0x00200000 - 0x005FFFFF** | **4MB** | **Flash Bank A** | Active/Standby |
| **0x00600000 - 0x009FFFFF** | **4MB** | **Flash Bank B** | Standby/Active |
| 0x00A00000 - 0x00FFFFFF | 6MB | Routing Storage | - |

### Bank Marker Storage
- **Address**: 0x000FF000
- **Value**: 
  - 0xAAAAAAAA = Bank A is active
  - 0xBBBBBBBB = Bank B is active

---

## 🔄 Download Flow with Routing

### Scenario 1: ZGW Software Update (Dual Bank)

```
VMG sends package (Target: ECU_091)
    ↓
ZGW receives UDS RequestDownload
    ↓
ZGW receives TransferData Block 1
    ├─ Parse Header
    ├─ Detect Target: ECU_091 (ZGW itself)
    ├─ Check Active Bank: A
    ├─ Select Standby Bank: B
    ├─ Erase Bank B (0x00600000)
    └─ Write to Bank B
    ↓
ZGW receives TransferData Block 2-N
    └─ Continue writing to Bank B
    ↓
ZGW receives RequestTransferExit
    ├─ Verify transfer complete
    ├─ Write Bank Marker (0xBBBBBBBB)
    ├─ Switch Active → Bank B
    └─ Response: SUCCESS (Reboot required)
    ↓
REBOOT
    └─ Boot from Bank B (New software!)
```

### Scenario 2: Zone ECU Software Update (Routing)

```
VMG sends package (Target: ECU_011)
    ↓
ZGW receives UDS RequestDownload
    ↓
ZGW receives TransferData Block 1
    ├─ Parse Header
    ├─ Detect Target: ECU_011 (Zone ECU)
    ├─ Allocate Route Buffer
    └─ Store to RAM buffer
    ↓
ZGW receives TransferData Block 2-N
    └─ Append to RAM buffer
    ↓
ZGW receives RequestTransferExit
    ├─ Verify transfer complete
    ├─ Store package to Flash @ 0x00A00000
    ├─ Initiate routing to ECU_011
    │   ├─ Option 1: Forward via DoIP
    │   ├─ Option 2: Forward via CAN (ISO 15765-2)
    │   └─ Option 3: Ethernet forwarding
    └─ Response: SUCCESS
    ↓
Zone ECU (ECU_011) receives package
    └─ Performs its own UDS download
```

---

## 🧪 Testing

### Test 1: ZGW Software Update (Dual Bank)

```bash
# Edit test_uds_download_with_header.py
TEST_TARGET_ECU = ECU_ID_ZGW  # 0x0091

# Run test
python test_uds_download_with_header.py
```

**Expected Output (UART Console):**
```
[SWPackage] Target ECU: ECU_091 (ZGW) (0x0091)
[SWPackage] ✓ TARGET: This ECU (ZGW ECU_091)
[SWPackage] Action: Update Standby Flash Bank
[SWPackage] Target Flash Bank: B (0x00600000)
[SWPackage] Erasing target Flash Bank...
[UDS 0x36] Block 1: 0 / 2048 bytes (0%) [Flash]
...
[UDS 0x37] Programmed Bank: B (0x00600000 - 0x00600800)
[FlashBank] NEW ACTIVE: Bank B (0x00600000)
[UDS 0x37] *** NEW SOFTWARE WILL BE ACTIVE AFTER REBOOT ***
```

### Test 2: Zone ECU Routing

```bash
# Edit test_uds_download_with_header.py
TEST_TARGET_ECU = ECU_ID_ZONE_1  # 0x0011

# Run test
python test_uds_download_with_header.py
```

**Expected Output (UART Console):**
```
[SWPackage] Target ECU: ECU_011 (Zone 1) (0x0011)
[SWPackage] → TARGET: Zone ECU (Routing Required)
[SWPackage] Action: Store for routing
[SWPackage] Route buffer allocated: 2112 bytes
[UDS 0x36] Block 1: 0 / 2048 bytes (0%) [Buffer]
...
[UDS 0x37] Target: Zone ECU (Routing required)
[UDS 0x37] Routing to: ECU_011 (Zone 1)
[Route] Storing package to Flash @ 0x00A00000
[Route] Package stored successfully!
[Route] TODO: Forward to Zone ECU via DoIP/CAN
```

---

## 📝 Implementation Notes

### ⚠️ Important Considerations

#### 1. **Flash Bank Switch Requires Reboot**
After successful software update to ZGW, a reboot is required to activate the new software. The bootloader must read the bank marker and boot from the correct bank.

#### 2. **Routing is Placeholder**
The current routing implementation stores the package to flash but doesn't actually forward it to the Zone ECU. You need to implement:
- DoIP forwarding via `DoIP_Client_SendDiagnostic()`
- CAN TP forwarding (ISO 15765-2)
- Or custom Ethernet forwarding

#### 3. **CRC Verification**
The header includes a CRC32 field, but verification is not yet implemented in `RequestTransferExit`. Add this for production:

```c
uint32 calculated_crc = SoftwarePackage_CalculateCRC32(payload_data, payload_size);
if (calculated_crc != g_download_session.sw_header.crc32) {
    // CRC mismatch!
    return NRC_GENERAL_PROGRAMMING_FAILURE;
}
```

#### 4. **Security**
Current implementation has NO authentication. For production:
- Implement SecurityAccess (UDS 0x27) before allowing download
- Verify digital signature in header (signature[3] field)
- Add firmware signing/verification

#### 5. **Flash Wear Leveling**
Dual bank flash helps reduce wear, but for long-term use, consider:
- Tracking bank switch count
- Flash health monitoring
- Bad block management

---

## 🔧 Code Integration Steps

### Step 1: Copy Enhanced Functions

Replace functions in `Libraries/DoIP/uds_download.c`:
- `UDS_Service_TransferData` → Use version from `uds_download_v2.c`
- `UDS_Service_RequestTransferExit` → Use version from `uds_download_v2.c`
- Add `UDS_Download_RouteToZoneECU` function

### Step 2: Initialize Flash Bank Manager

Add to `Cpu0_Main.c` after SPI Flash init:

```c
/* Initialize Flash Bank Manager */
FlashBank_Init();
sendUARTMessage("[FlashBank] Dual Bank Manager initialized\r\n", 44);
```

### Step 3: Update Download Session Init

In `UDS_Download_Init()`, add:

```c
g_download_session.header_received = FALSE;
g_download_session.is_for_this_ecu = FALSE;
g_download_session.target_flash_bank = FLASH_BANK_UNKNOWN;
g_download_session.route_buffer = NULL;
g_download_session.route_buffer_size = 0;
```

### Step 4: Build and Test

```bash
cd "C:\Users\user\AURIX-v1.10.24-workspace\Zonal_Gateway"
"TriCore Debug (TASKING)\make.bat"
```

---

## 🚀 Future Enhancements

### 1. **Multi-Hop Routing**
Use `hop_count` field to route through intermediate gateways:
```
VMG → ZGW (hop 0) → ZGW_2 (hop 1) → ECU_011 (hop 2)
```

### 2. **Batch Updates**
Use `sequence_number` for multi-package updates:
```
Package 1 (seq 1): Bootloader
Package 2 (seq 2): Application
Package 3 (seq 3): Calibration
```

### 3. **Compression Support**
Implement on-the-fly decompression:
```c
if (header->compression == 1) {
    decompress_gzip(compressed_data, decompressed_buffer);
}
```

### 4. **Progress Reporting**
Send periodic status to VMG via UDS 0x22 (ReadDataByIdentifier):
```
DID 0xF1C0: Download Progress (%)
DID 0xF1C1: Active Flash Bank
DID 0xF1C2: Routing Status
```

### 5. **Rollback Capability**
If new software fails, revert to previous bank:
```c
if (self_test_failed()) {
    FlashBank_SwitchActive();  // Revert to old bank
    reboot();
}
```

---

## 📊 Performance

### Download Time Estimates

| Package Size | Flash Write | Routing Store | Total |
|--------------|-------------|---------------|-------|
| 100 KB | ~45 sec | ~45 sec | ~1.5 min |
| 1 MB | ~8 min | ~8 min | ~16 min |
| 4 MB | ~35 min | ~35 min | ~70 min |

### Optimization Tips

1. **Increase Block Size**: Up to 4KB (current: 256B)
2. **Reduce UART Logging**: Disable verbose logs in production
3. **Use DMA**: Offload flash operations to DMA
4. **Parallel Updates**: Update multiple Zone ECUs simultaneously

---

## 🆘 Troubleshooting

### Problem: "Invalid magic" error
**Cause**: Header not properly formatted  
**Fix**: Verify Python script generates correct header (0x53575047)

### Problem: "Target: UNKNOWN"
**Cause**: Invalid `target_ecu_id` in header  
**Fix**: Use correct ECU IDs (0x0091, 0x0011, etc.)

### Problem: Bank switch doesn't take effect
**Cause**: Bootloader not reading bank marker  
**Fix**: Implement bootloader logic to check 0x000FF000

### Problem: Routing fails
**Cause**: Route function is placeholder  
**Fix**: Implement actual DoIP/CAN forwarding

---

## 📚 References

- **ISO 14229-1**: UDS Services (0x34, 0x36, 0x37)
- **ISO 13400**: DoIP Protocol
- **ISO 15765-2**: CAN Transport Protocol
- **S25FL512S Datasheet**: Flash memory specifications

---

**Status**: ✅ Core Logic Implemented (Integration Required)  
**Last Updated**: 2025-11-06

---

## 📝 Quick Reference

### ECU IDs
```c
#define ECU_ID_ZGW          0x0091    // Zonal Gateway
#define ECU_ID_ZONE_1       0x0011    // Zone ECU 1
#define ECU_ID_ZONE_2       0x0012    // Zone ECU 2
#define ECU_ID_BROADCAST    0xFFFF    // All ECUs
```

### Flash Banks
```c
Bank A: 0x00200000 - 0x005FFFFF (4MB)
Bank B: 0x00600000 - 0x009FFFFF (4MB)
Marker: 0x000FF000 (4 bytes)
```

### Test Commands
```bash
# ZGW Update Test
python test_uds_download_with_header.py

# Zone ECU Routing Test
# (Edit TEST_TARGET_ECU = ECU_ID_ZONE_1 first)
python test_uds_download_with_header.py
```

