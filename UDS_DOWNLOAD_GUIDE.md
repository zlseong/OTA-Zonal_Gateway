# UDS Download to SPI Flash - User Guide

## Overview

This implementation enables **firmware/software download** from VMG (Vehicle Management Gateway) to the Zonal Gateway's **SPI Flash (S25FL512S)** via **UDS over DoIP**.

### Key Features
- ✅ UDS Services: 0x34 (RequestDownload), 0x36 (TransferData), 0x37 (RequestTransferExit)
- ✅ Flash Storage: 15MB download area (1MB-16MB)
- ✅ Block Transfer: 256 bytes per block (configurable)
- ✅ Automatic Flash Erase before write
- ✅ Progress monitoring via UART logs
- ✅ Python test script included

---

## Architecture

```
VMG (192.168.1.100)
    ↓ DoIP (TCP 13400)
Zonal Gateway (192.168.1.10)
    ↓ UDS Handler
    ├─ 0x34 RequestDownload → Erase Flash Area
    ├─ 0x36 TransferData    → Write Blocks to Flash
    └─ 0x37 TransferExit    → Complete & Verify
         ↓
SPI Flash S25FL512S (64MB)
    └─ Download Area: 0x00100000 - 0x00FFFFFF (15MB)
```

---

## Flash Memory Layout

| Address Range | Size | Purpose |
|---------------|------|---------|
| 0x00000000 - 0x000FFFFF | 1MB | Reserved (Boot/Config) |
| **0x00100000 - 0x00FFFFFF** | **15MB** | **Download Area** |

**Note**: Current implementation uses 3-byte address mode (16MB addressable)

---

## UDS Download Sequence

### 1. **RequestDownload (0x34)**
```
Request:
  [0x34] [dataFormat] [addrLenFormat] [address] [size]
  
Example:
  34 00 44 00100000 00000800  (Download 2KB to 0x00100000)
  
Response (Positive):
  74 20 0100  (Max block: 256 bytes)
  
Response (Negative):
  7F 34 [NRC]
```

**What it does:**
- Validates address range (must be in 0x00100000-0x00FFFFFF)
- Erases flash area (256KB blocks)
- Initializes download session

### 2. **TransferData (0x36)**
```
Request:
  [0x36] [blockCounter] [data...]
  
Example:
  36 01 [256 bytes data]  (Block 1)
  36 02 [256 bytes data]  (Block 2)
  ...
  
Response (Positive):
  76 [blockCounter]
  
Response (Negative):
  7F 36 [NRC]
```

**What it does:**
- Verifies block sequence counter
- Writes data to SPI Flash
- Updates download progress

### 3. **RequestTransferExit (0x37)**
```
Request:
  [0x37]
  
Response (Positive):
  77
  
Response (Negative):
  7F 37 [NRC]
```

**What it does:**
- Verifies all data received
- Completes download session
- (Optional: CRC verification)

---

## Testing with Python Script

### Prerequisites
```bash
pip install socket  # Built-in, no install needed
```

### Run Test
```bash
python test_uds_download.py
```

### Test Output
```
==================================================
UDS Download Test - VMG Simulator
==================================================
Target: 192.168.1.10:13400
VMG Address: 0x0E00
ZGW Address: 0x0100

Generating 2048 bytes test data...
  Pattern: 0x00 0x01 ... 0xFE 0xFF

Connecting to Zonal Gateway...
[Connected]

==================================================
[UDS 0x34] RequestDownload
  Address: 0x00100000
  Size: 2048 bytes (2 KB)
==================================================
[TX] Sending 27 bytes...
[RX] DoIP Header: Type=0x8001, Len=7
[RX] Received 7 bytes payload
[SUCCESS] RequestDownload accepted!
  Max block length: 256 bytes

==================================================
[UDS 0x36] TransferData
==================================================
  Block 1: 256 bytes (offset 0)
[TX] Sending 274 bytes...
  Block 2: 256 bytes (offset 256)
  ...
  Block 8: 256 bytes (offset 1792)

  Total: 2048 bytes transferred

==================================================
[UDS 0x37] RequestTransferExit
==================================================
[TX] Sending 13 bytes...
[SUCCESS] TransferExit complete!

==================================================
SOFTWARE DOWNLOAD TEST COMPLETE!
==================================================
✓ Downloaded 2048 bytes to Flash @ 0x00100000
✓ Data stored in SPI Flash S25FL512S

Connection closed.
```

### UART Console Output (Zonal Gateway)
```
[UDS Download] Module initialized
[UDS] RX: SID=0x34, SA=0x0E00, TA=0x0100, Len=10
[UDS 0x34] RequestDownload: Addr=0x00100000, Size=2048 bytes
[UDS Download] Erasing Flash: 0x00100000, Size: 2 KB
  Erasing block 1/1 at 0x00100000...
[UDS Download] Flash erase complete!
[UDS 0x34] Download session started (Max block: 256 bytes)

[UDS] RX: SID=0x36, SA=0x0E00, TA=0x0100, Len=257
[UDS 0x36] Block 1: 0 / 2048 bytes (0%)
[UDS 0x36] Block 8: 2048 / 2048 bytes (100%)

[UDS] RX: SID=0x37, SA=0x0E00, TA=0x0100, Len=0
[UDS 0x37] RequestTransferExit: Received 2048 / 2048 bytes
[UDS 0x37] Flash write complete (verification skipped for speed)
[UDS 0x37] ========================================
[UDS 0x37] SOFTWARE DOWNLOAD SUCCESS!
[UDS 0x37] Flash: 0x00100000 - 0x00100800 (2048 bytes)
[UDS 0x37] ========================================
```

---

## Error Handling

### Common Negative Response Codes (NRC)

| NRC | Name | Meaning | Solution |
|-----|------|---------|----------|
| 0x10 | GeneralReject | Generic error | Check logs |
| 0x11 | ServiceNotSupported | Service not implemented | Check UDS handler |
| 0x13 | IncorrectMessageLength | Wrong request format | Check request structure |
| 0x22 | ConditionsNotCorrect | Download already active | Reset session |
| 0x24 | RequestSequenceError | No active session | Send RequestDownload first |
| 0x31 | RequestOutOfRange | Address invalid | Check address range |
| 0x70 | UploadDownloadNotAccepted | Flash not ready | Check flash init |
| 0x72 | GeneralProgrammingFailure | Flash write error | Check hardware |
| 0x73 | WrongBlockSequenceCounter | Block order mismatch | Restart download |

### Troubleshooting

**Problem: RequestDownload rejected (NRC 0x31)**
- **Cause**: Address outside allowed range
- **Fix**: Use address 0x00100000 - 0x00FFFFFF

**Problem: TransferData fails (NRC 0x73)**
- **Cause**: Block sequence counter mismatch
- **Fix**: Ensure blocks sent in order (1, 2, 3, ...)

**Problem: Flash write timeout**
- **Cause**: Flash erase too slow
- **Fix**: Increase timeout in `spi_flash_s25fl512s.h`

---

## Configuration

### Modify Download Area Size

Edit `Libraries/DoIP/uds_download.h`:

```c
#define FLASH_AREA_DOWNLOAD_START       0x00100000  /* Start address */
#define FLASH_AREA_DOWNLOAD_SIZE        0x00F00000  /* 15MB */
```

### Modify Block Size

Edit `Libraries/DoIP/uds_download.h`:

```c
#define FLASH_WRITE_MAX_SIZE            256  /* Max bytes per block */
```

### Enable Verification (Slow!)

Edit `Libraries/DoIP/uds_download.c`, in `UDS_Service_RequestTransferExit()`:

```c
/* Uncomment to enable verification */
// if (!Flash_VerifyData(address, expected_data, length)) {
//     return FALSE;
// }
```

---

## Code Structure

```
Libraries/DoIP/
├── uds_download.h         - Download API definitions
├── uds_download.c         - Download implementation
├── uds_handler.h          - UDS service IDs
└── uds_handler.c          - UDS service dispatcher

Libraries/SPI_Flash/
├── spi_flash_s25fl512s.h  - Flash driver API
└── spi_flash_s25fl512s.c  - Flash driver implementation

test_uds_download.py       - Python test script (VMG simulator)
```

---

## Future Enhancements

### 1. **4-byte Address Mode (Full 64MB)**
Currently limited to 16MB (3-byte mode). To access full 64MB:
- Enter 4-byte address mode (0xB7)
- Use 4-byte address commands (0x13, 0x12, 0x21)
- Update address validation

### 2. **CRC Verification**
Add CRC32 calculation for downloaded data:
- Calculate CRC during transfer
- Compare with VMG-provided CRC
- Report mismatch errors

### 3. **Compression Support**
Add on-the-fly decompression:
- Support `dataFormatId` in RequestDownload
- Decompress during TransferData
- Store decompressed data to Flash

### 4. **Multi-Partition Support**
Define multiple download areas:
- Application firmware
- Configuration data
- Calibration data
- Boot loader

### 5. **Security Access (0x27)**
Add authentication before download:
- Implement SecurityAccess service
- Require seed/key exchange
- Prevent unauthorized updates

---

## Performance

### Typical Download Speed

| Data Size | Time | Speed |
|-----------|------|-------|
| 2 KB | ~1 sec | 2 KB/s |
| 100 KB | ~45 sec | 2.2 KB/s |
| 1 MB | ~8 min | 2 KB/s |
| 15 MB | ~2 hours | 2 KB/s |

**Bottlenecks:**
- Flash erase time (256KB = ~2 seconds)
- Flash write time (256B = ~5ms)
- DoIP/TCP overhead
- UART logging

**Optimization:**
- Increase block size (up to 4KB)
- Reduce UART logging
- Use DMA for flash operations

---

## Safety & Production Notes

### ⚠️ Critical Warnings

1. **Flash Erase is Destructive**
   - Always backup critical data
   - Don't download to boot area (0x00000000-0x000FFFFF)

2. **Power Loss During Download**
   - Can corrupt flash data
   - Implement checksum verification
   - Use atomic write operations

3. **Address Range Validation**
   - Always validate requested address
   - Prevent overflow to critical areas
   - Check for wrap-around bugs

4. **Security Considerations**
   - Current implementation has NO authentication
   - Add SecurityAccess (0x27) for production
   - Implement firmware signature verification

---

## References

- **ISO 14229-1**: Unified Diagnostic Services (UDS)
- **ISO 13400**: DoIP (Diagnostics over IP)
- **S25FL512S Datasheet**: Spansion 64MB SPI Flash
- **MikroE Flash 4 Click**: Hardware reference

---

## Support

For issues or questions:
1. Check UART console logs
2. Review error codes in this guide
3. Verify flash hardware connections
4. Test with `test_uds_download.py`

**Status**: ✅ Fully Functional (v25)  
**Last Updated**: 2025-11-06

