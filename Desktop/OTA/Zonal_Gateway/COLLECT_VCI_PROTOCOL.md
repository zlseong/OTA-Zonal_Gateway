# VCI Collection Protocol (Request-Response Model)

## Overview
This document defines the **VCI (Vehicle Configuration Information) Collection Protocol** using a **Request-Response Model** with **UDS over DoIP**. The protocol specifies how VCI information is requested and collected from ECUs through the Zonal Gateway to the VMG (Vehicle Master Gateway).

---

## Protocol Architecture

### Communication Model: **Pull-Based (Request-Response)**

```
External Server
    ↓ (1) VCI Request
┌─────────────┐
│     VMG     │
└─────────────┘
    ↓ (2) VCI Request (UDS 0x22 DID 0xF195)
┌─────────────┐
│  Zonal GW   │
│  (ECU_091)  │
└─────────────┘
    ↓ (3) VCI Request (UDS 0x22 DID 0xF194)
┌─────────────┐
│  ECU_011    │
│  ECU_01x    │
│  ...        │
└─────────────┘
    ↑ (4) VCI Response (UDS 0x62 + VCI Data)
┌─────────────┐
│  Zonal GW   │
│  (Collect   │
│   All VCI)  │
└─────────────┘
    ↑ (5) Consolidated VCI Response (UDS 0x62 + All VCI)
┌─────────────┐
│     VMG     │
└─────────────┘
    ↑ (6) VCI Response
External Server
```

---

## Detailed Message Flow

### Step 1: External Server → VMG
**Trigger:** Server requests VCI information

```
HTTP/REST API Request:
GET /api/vehicle/vci
```

---

### Step 2: VMG → Zonal Gateway
**UDS Request over DoIP**

```
DoIP Header:
  Protocol Version:         0x02
  Inverse Protocol Version: 0xFD
  Payload Type:             0x8001 (Diagnostic Message)
  Payload Length:           7 bytes

DoIP Routing:
  Source Address:           0x0E00 (VMG)
  Target Address:           0x0100 (Zonal Gateway)

UDS Request:
  Service ID:               0x22 (ReadDataByIdentifier)
  Data Identifier (DID):    0xF195 (Consolidated VCI)
  
Total Message: [02 FD 80 01 00 00 00 07] [0E 00 01 00] [22 F1 95]
```

**DID Definition:**
- `0xF195` - Consolidated VCI (All ECUs in zone)

---

### Step 3: Zonal Gateway → Zone ECU (ECU_011)
**UDS Request over DoIP (or CAN)**

```
DoIP Header:
  Protocol Version:         0x02
  Inverse Protocol Version: 0xFD
  Payload Type:             0x8001 (Diagnostic Message)
  Payload Length:           7 bytes

DoIP Routing:
  Source Address:           0x0100 (Zonal Gateway)
  Target Address:           0x0011 (ECU_011)

UDS Request:
  Service ID:               0x22 (ReadDataByIdentifier)
  Data Identifier (DID):    0xF194 (Individual ECU VCI)
  
Total Message: [02 FD 80 01 00 00 00 07] [01 00 00 11] [22 F1 94]
```

**DID Definition:**
- `0xF194` - Individual ECU VCI (Single ECU)

**Note:** Zonal Gateway sends this request to **ALL Zone ECUs** (ECU_011, ECU_012, ... ECU_01N)

---

### Step 4: Zone ECU (ECU_011) → Zonal Gateway
**UDS Response**

```
DoIP Header:
  Protocol Version:         0x02
  Inverse Protocol Version: 0xFD
  Payload Type:             0x8001 (Diagnostic Message)
  Payload Length:           55 bytes (4 routing + 3 UDS + 48 VCI)

DoIP Routing:
  Source Address:           0x0011 (ECU_011)
  Target Address:           0x0100 (Zonal Gateway)

UDS Response:
  Service ID:               0x62 (Positive Response for 0x22)
  Data Identifier (DID):    0xF194 (Echo)
  VCI Data:                 48 bytes

VCI Data Structure (48 bytes):
  ECU ID:       "ECU_011\0\0\0\0\0\0\0\0\0"  (16 bytes, ASCII)
  SW Version:   "0.0.0\0\0\0"                 (8 bytes, ASCII)
  HW Version:   "0.0.0\0\0\0"                 (8 bytes, ASCII)
  Serial Num:   "011000001\0\0\0\0\0\0\0"    (16 bytes, ASCII)

Total Message: [02 FD 80 01 00 00 00 37] [00 11 01 00] [62 F1 94] [VCI_DATA_48_BYTES]
```

**Zonal Gateway Action:**
1. Receives VCI from ECU_011 ✅
2. Stores in VCI database
3. Checks if all Zone ECUs responded
4. If `collected_count >= MAX_ZONE_ECUS` → Proceed to Step 5

---

### Step 5: Zonal Gateway → VMG
**Consolidated VCI Response**

```
DoIP Header:
  Protocol Version:         0x02
  Inverse Protocol Version: 0xFD
  Payload Type:             0x8001 (Diagnostic Message)
  Payload Length:           104 bytes (4 routing + 4 UDS header + 1 count + 96 VCI data)

DoIP Routing:
  Source Address:           0x0100 (Zonal Gateway)
  Target Address:           0x0E00 (VMG)

UDS Response:
  Service ID:               0x62 (Positive Response for 0x22)
  Data Identifier (DID):    0xF195 (Consolidated VCI)
  ECU Count:                0x02 (2 ECUs: ECU_011 + ECU_091)

VCI Data Array:
  ECU #1: ECU_011 VCI (48 bytes)
    - ECU ID:     "ECU_011\0\0\0\0\0\0\0\0\0"
    - SW Version: "0.0.0\0\0\0"
    - HW Version: "0.0.0\0\0\0"
    - Serial:     "011000001\0\0\0\0\0\0\0"
  
  ECU #2: ECU_091 VCI (48 bytes)
    - ECU ID:     "ECU_091\0\0\0\0\0\0\0\0\0"
    - SW Version: "0.0.0\0\0\0"
    - HW Version: "0.0.0\0\0\0"
    - Serial:     "091000001\0\0\0\0\0\0\0"

Total Message: 
  [02 FD 80 01 00 00 00 68] (DoIP Header)
  [01 00 0E 00]             (Routing)
  [62 F1 95]                (UDS Response + DID)
  [02]                      (ECU Count)
  [ECU_011_VCI_48_BYTES]
  [ECU_091_VCI_48_BYTES]
```

---

### Step 6: VMG → External Server
**REST API Response**

```json
{
  "status": "success",
  "ecu_count": 2,
  "vci_list": [
    {
      "ecu_id": "ECU_011",
      "sw_version": "0.0.0",
      "hw_version": "0.0.0",
      "serial_number": "011000001"
    },
    {
      "ecu_id": "ECU_091",
      "sw_version": "0.0.0",
      "hw_version": "0.0.0",
      "serial_number": "091000001"
    }
  ]
}
```

---

## UDS Data Identifier (DID) Definitions

### Standard DIDs (ISO 14229)

| DID | Name | Description | Size |
|-----|------|-------------|------|
| `0xF190` | VIN | Vehicle Identification Number | 17 bytes |
| `0xF191` | ECU Hardware Number | Hardware part number | Variable |
| `0xF192` | System Supplier ID | Supplier code | Variable |
| `0xF193` | ECU Manufacturing Date | Date code | Variable |
| `0xF194` | **Individual ECU VCI** | Single ECU VCI (Custom) | 48 bytes |
| `0xF195` | **Consolidated VCI** | All Zone ECUs VCI (Custom) | Variable |
| `0xF19E` | Application Software Version | SW version string | Variable |
| `0xF1A0` | **ECU Health Status** | Dynamic health info (Custom) | Variable |

### Custom DIDs (Project Specific)

**DID 0xF194 - Individual ECU VCI**
```c
struct VCI_Info {
    char ecu_id[16];        // "ECU_011", "ECU_091", etc.
    char sw_version[8];     // "0.0.0"
    char hw_version[8];     // "0.0.0"
    char serial_num[16];    // "011000001", "091000001"
};
// Total: 48 bytes
```

**DID 0xF195 - Consolidated VCI**
```c
struct Consolidated_VCI {
    uint8 ecu_count;                        // Number of ECUs
    VCI_Info vci_array[MAX_ZONE_ECUS];     // Array of VCI_Info
};
// Total: 1 + (48 * ecu_count) bytes
```

---

## ECU ID Naming Convention

### Zonal Gateway: `ECU_09x`
- `ECU_091` - Zonal Gateway #1
- `ECU_092` - Zonal Gateway #2

### Zone ECUs: `ECU_0xy`
- `ECU_011` - Zone 1, ECU #1 (Body Domain)
- `ECU_012` - Zone 1, ECU #2
- `ECU_021` - Zone 2, ECU #1 (ADAS Domain)

### Serial Number Format: `[ECU_NUM][SEQUENCE]`
- `091000001` - ZGW #1, Unit #1
- `011000001` - ECU_011, Unit #1

---

## Timing Requirements

| Phase | Timeout | Action on Timeout |
|-------|---------|-------------------|
| **Step 2-3**: VMG → ZGW | 3 seconds | Return error to Server |
| **Step 3-4**: ZGW → ECU | 2 seconds per ECU | Send partial VCI |
| **Step 5-6**: ZGW → VMG | 3 seconds | Retry 3 times |

**Total Max Latency:** ~8 seconds (for 1 Zone ECU)

---

## Implementation Pseudocode

### Zonal Gateway (C Code)

```c
/* VCI Collection State */
#define MAX_ZONE_ECUS 1
VCI_Info g_vci_database[MAX_ZONE_ECUS + 1];  // +1 for ZGW itself
uint8 g_vci_count = 0;
boolean g_vci_collection_complete = FALSE;

/* Step 3: Handle VCI Request from VMG */
void OnUDS_Request_VCI(uint16 source_addr, uint16 did)
{
    if (did == 0xF195)  // Consolidated VCI Request
    {
        // Reset collection
        g_vci_count = 0;
        g_vci_collection_complete = FALSE;
        
        // Request VCI from all Zone ECUs
        for (uint8 i = 0; i < MAX_ZONE_ECUS; i++)
        {
            UDS_SendRequest(ZONE_ECU_ADDRESS[i], 0x22, 0xF194);
        }
        
        // Start timeout timer (2 seconds)
        StartVCICollectionTimer();
    }
}

/* Step 4: Handle VCI Response from Zone ECU */
void OnUDS_Response_VCI(uint16 source_addr, const VCI_Info* vci)
{
    // Store VCI
    if (g_vci_count < MAX_ZONE_ECUS)
    {
        memcpy(&g_vci_database[g_vci_count], vci, sizeof(VCI_Info));
        g_vci_count++;
        
        // Check if all ECUs responded
        if (g_vci_count >= MAX_ZONE_ECUS)
        {
            // Add ZGW own VCI
            VCI_Info zgw_vci = {
                .ecu_id = "ECU_091",
                .sw_version = "0.0.0",
                .hw_version = "0.0.0",
                .serial_num = "091000001"
            };
            memcpy(&g_vci_database[g_vci_count], &zgw_vci, sizeof(VCI_Info));
            g_vci_count++;
            
            g_vci_collection_complete = TRUE;
            
            // Step 5: Send to VMG
            SendConsolidatedVCI_ToVMG(g_vci_database, g_vci_count);
        }
    }
}

/* Step 5: Send Consolidated VCI to VMG */
void SendConsolidatedVCI_ToVMG(const VCI_Info* vci_array, uint8 count)
{
    uint8 buffer[256];
    uint16 offset = 0;
    
    // DoIP Header (8 bytes)
    buffer[offset++] = 0x02;  // Protocol Version
    buffer[offset++] = 0xFD;  // Inverse Version
    buffer[offset++] = 0x80;  // Payload Type High
    buffer[offset++] = 0x01;  // Payload Type Low (0x8001)
    
    uint32 payload_len = 4 + 3 + 1 + (count * sizeof(VCI_Info));
    buffer[offset++] = (payload_len >> 24) & 0xFF;
    buffer[offset++] = (payload_len >> 16) & 0xFF;
    buffer[offset++] = (payload_len >> 8) & 0xFF;
    buffer[offset++] = payload_len & 0xFF;
    
    // DoIP Routing (4 bytes)
    buffer[offset++] = 0x01;  // Source Address High (0x0100)
    buffer[offset++] = 0x00;  // Source Address Low
    buffer[offset++] = 0x0E;  // Target Address High (0x0E00)
    buffer[offset++] = 0x00;  // Target Address Low
    
    // UDS Response (3 bytes)
    buffer[offset++] = 0x62;  // Service ID (Positive Response)
    buffer[offset++] = 0xF1;  // DID High
    buffer[offset++] = 0x95;  // DID Low (0xF195)
    
    // ECU Count (1 byte)
    buffer[offset++] = count;
    
    // VCI Data Array
    for (uint8 i = 0; i < count; i++)
    {
        memcpy(&buffer[offset], &vci_array[i], sizeof(VCI_Info));
        offset += sizeof(VCI_Info);
    }
    
    // Send via TCP
    DoIP_Send(buffer, offset);
}
```

---

## Error Handling

### Negative Response Codes (NRC)

| NRC | Code | Description | Action |
|-----|------|-------------|--------|
| **Service Not Supported** | 0x11 | ECU doesn't support 0x22 | Skip this ECU |
| **Sub-Function Not Supported** | 0x12 | DID not supported | Use alternative DID |
| **Request Out of Range** | 0x31 | Invalid DID | Check DID definition |
| **Conditions Not Correct** | 0x22 | ECU not ready | Retry after 1s |
| **Response Pending** | 0x78 | Processing... | Wait for final response |

### Timeout Scenarios

| Scenario | Recovery |
|----------|----------|
| **No response from ECU** | Send partial VCI (ZGW only) |
| **Partial ECU responses** | Send available VCI |
| **VMG connection lost** | Cache VCI, retry when reconnected |

---

## Testing Checklist

- [ ] VMG sends VCI request to ZGW
- [ ] ZGW forwards request to ECU_011
- [ ] ECU_011 responds with valid VCI
- [ ] ZGW collects and adds own VCI
- [ ] ZGW sends consolidated VCI to VMG
- [ ] VMG parses and displays both VCIs
- [ ] Timeout handling (ECU doesn't respond)
- [ ] Multiple ECU support (when `MAX_ZONE_ECUS > 1`)

---

## Revision History

| Version | Date | Author | Changes |
|---------|------|--------|---------|
| **3.0** | 2025-11-04 | Zonal Gateway Team | **Request-Response Model** (Pull-based VCI collection) |
| 2.0 | 2025-11-04 | Zonal Gateway Team | Push-based VCI protocol |
| 1.0 | 2025-11-04 | Zonal Gateway Team | Initial VCI naming convention |

---

## References

- **ISO 14229-1** - Unified Diagnostic Services (UDS) - Part 1: Application Layer
- **ISO 13400-2** - DoIP - Part 2: Transport Protocol and Network Layer Services
- **ISO 15765-2** - Diagnostic Communication over CAN (DoCAN)
- **SAE J1979** - E/E Diagnostic Test Modes

---

## Appendix: Hex Message Examples

### Example 1: VMG → ZGW (Request VCI)
```
02 FD 80 01 00 00 00 07 0E 00 01 00 22 F1 95
│  │  │  │  └──────────┘ │     │     └─────┘
│  │  │  │     Payload   │     │      UDS Request
│  │  │  │     Length=7  │     │      (Read DID 0xF195)
│  │  │  │                │     └─ Target: 0x0100 (ZGW)
│  │  │  └─ Payload Type │
│  │  │     0x8001        └─ Source: 0x0E00 (VMG)
│  │  └─ Inverse Version
│  └─ Protocol Version
└─ 0x02
```

### Example 2: ECU_011 → ZGW (Response VCI)
```
02 FD 80 01 00 00 00 37 00 11 01 00 62 F1 94 45 43 55 5F 30 31 31 00 ...
│  │  │  │  └──────────┘ │     │     └─────┘ └────────────────────┘
│  │  │  │     Payload   │     │      UDS     VCI Data (48 bytes)
│  │  │  │     Length=55 │     │      Resp    "ECU_011..."
│  │  │  │                │     └─ Target: 0x0100 (ZGW)
│  │  │  └─ Diagnostic   └─ Source: 0x0011 (ECU_011)
│  │  │     Message
│  │  └─ 0x8001
│  └─ 0xFD
└─ 0x02
```

### Example 3: ZGW → VMG (Consolidated VCI Response)
```
02 FD 80 01 00 00 00 68 01 00 0E 00 62 F1 95 02 [ECU_011_VCI] [ECU_091_VCI]
│  │  │  │  └──────────┘ │     │     └─────┘ │  └──────────┘ └──────────┘
│  │  │  │     Payload   │     │      UDS    │   48 bytes     48 bytes
│  │  │  │     Length    │     │      Resp   └─ Count = 2
│  │  │  │     =104      │     └─ Target: VMG
│  │  │  │                └─ Source: ZGW
│  │  │  └─ 0x8001
│  │  └─ 0xFD
└─ 0x02
```

---

**END OF DOCUMENT**
