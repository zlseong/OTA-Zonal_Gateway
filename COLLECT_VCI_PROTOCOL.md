# Collect VCI Protocol

## Overview
This document defines the **Collect VCI (Vehicle Configuration Information) Protocol** used in the Zonal Gateway architecture. The protocol specifies how ECUs collect and report their configuration information to the Zonal Gateway, which then consolidates and forwards this information to the VMG (Vehicle Master Gateway).

---

## Protocol Trigger

**Trigger Event:** Vehicle Power ON

When the vehicle is powered on, the following sequence occurs:
1. All Zone ECUs initialize and collect their VCI information
2. Each ECU sends its VCI to the Zonal Gateway via **UDS messages**
3. Zonal Gateway collects VCI from all Zone ECUs
4. Zonal Gateway adds its own VCI to the collection
5. Zonal Gateway sends the consolidated VCI report to the VMG via **DoIP**

---

## Communication Flow

```
Power ON
    │
    ├─> Zone ECU #1 (ECU_011)
    │       │
    │       └─> [UDS] Send VCI to Zonal Gateway (ECU_091)
    │
    ├─> Zone ECU #2 (ECU_012)
    │       │
    │       └─> [UDS] Send VCI to Zonal Gateway (ECU_091)
    │
    ├─> Zone ECU #N (ECU_01N)
    │       │
    │       └─> [UDS] Send VCI to Zonal Gateway (ECU_091)
    │
    └─> Zonal Gateway (ECU_091)
            │
            ├─> Collect VCI from all Zone ECUs
            ├─> Add own VCI (ECU_091)
            └─> [DoIP] Send consolidated VCI to VMG
```

---

## ECU ID Naming Convention

### Zonal Gateway ID Format: `ECU_09x`

- **09**: Indicates Zonal Gateway
- **x**: Zonal Gateway instance number (0-9)

**Example:**
- `ECU_091` - Zonal Gateway #1
- `ECU_092` - Zonal Gateway #2

### Zone ECU ID Format: `ECU_0xy`

- **0**: Fixed prefix for ECUs in Zone 0
- **x**: Zone number (0-9)
- **y**: ECU number within the zone (0-9)

**Example:**
- `ECU_011` - Zone 1, ECU #1 (Body Domain - Left Front Door)
- `ECU_012` - Zone 1, ECU #2 (Body Domain - Right Front Door)
- `ECU_021` - Zone 2, ECU #1 (ADAS Domain - Front Camera)
- `ECU_031` - Zone 3, ECU #1 (Telematics Domain - TCU)

---

## Current System Architecture

### Hardware Configuration

| Component | ECU ID | Description |
|-----------|--------|-------------|
| **Zonal Gateway** | `ECU_091` | TC375 Main Controller |
| **Target ECU** | `ECU_011` | Zone 1 - Body Domain ECU #1 |

### Communication Protocol Stack

| Layer | Zonal Gateway ↔ Zone ECU | Zonal Gateway ↔ VMG |
|-------|---------------------------|---------------------|
| **Application** | VCI Collection | VCI Reporting |
| **Protocol** | **UDS (ISO 14229)** | **DoIP (ISO 13400)** |
| **Transport** | CAN / CAN-FD | TCP/IP |
| **Physical** | CAN Bus | Ethernet |

---

## VCI Data Structure

### VCI Information Fields

Each ECU provides the following VCI information:

```c
typedef struct {
    char ecu_id[16];        // ECU ID (e.g., "ECU_091", "ECU_011")
    char sw_version[8];     // Software version (e.g., "1.2.3")
    char hw_version[8];     // Hardware version (e.g., "2.0.1")
    char serial_num[16];    // Serial number (e.g., "091000001")
} VCI_Info;
```

### Serial Number Format

**Pattern:** `[ECU_Number][Sequence]`

- **ECU Number**: Last 3 digits of ECU ID (e.g., `091`, `011`)
- **Sequence**: 6-digit sequential number

**Examples:**
- `091000001` - Zonal Gateway #1
- `011000001` - Zone 1 ECU #1 (first unit)
- `011000002` - Zone 1 ECU #1 (second unit)

---

## UDS Message Format (ECU → Zonal Gateway)

### Service ID: 0x22 (ReadDataByIdentifier)

**Request (from Zonal Gateway):**
```
Service ID: 0x22
Data Identifier: 0xF187 (VCI Information)
```

**Response (from Zone ECU):**
```
Service ID: 0x62
Data Identifier: 0xF187
VCI Data:
  - ECU ID: 16 bytes (ASCII string)
  - SW Version: 8 bytes (ASCII string)
  - HW Version: 8 bytes (ASCII string)
  - Serial Number: 16 bytes (ASCII string)
Total: 48 bytes
```

### Alternative: Active VCI Push (ECU → Zonal Gateway)

**Custom Service: 0x2F (InputOutputControlByIdentifier)**
```
Service ID: 0x2F
Data Identifier: 0xF187
Control Parameter: 0x03 (Report VCI)
VCI Data: 48 bytes
```

---

## DoIP Message Format (Zonal Gateway → VMG)

### Payload Type: 0x8001 (Diagnostic Message)

**DoIP Header:**
```
Protocol Version: 0x02
Inverse Protocol Version: 0xFD
Payload Type: 0x8001 (Diagnostic Message)
Payload Length: Variable
```

**Payload:**
```
Source Address: 0x0100 (Zonal Gateway)
Target Address: 0x0200 (VMG)
UDS Message:
  Service ID: 0x2E (WriteDataByIdentifier)
  Data Identifier: 0xF187
  ECU Count: 1 byte (N)
  VCI Data Array: N * 48 bytes
    - ECU_091 VCI: 48 bytes
    - ECU_011 VCI: 48 bytes
    - ... (other ECUs)
```

---

## VCI Collection Process

### State Machine

```
[IDLE] 
  │
  ├─> Power ON
  │
[WAIT_FOR_ECU_VCI]
  │
  ├─> Receive VCI from ECU_011 via UDS
  ├─> Receive VCI from ECU_01x via UDS (future)
  │
[ALL_ECU_VCI_COLLECTED]
  │
  ├─> Add Zonal Gateway VCI (ECU_091)
  │
[SEND_TO_VMG]
  │
  ├─> Consolidate all VCI data
  ├─> Send via DoIP to VMG
  │
[VCI_REPORTED]
  │
  └─> Wait for next power cycle
```

### Timing Requirements

| Event | Timeout | Action on Timeout |
|-------|---------|-------------------|
| **ECU VCI Collection** | 5 seconds | Report with available VCI only |
| **DoIP Connection** | 10 seconds | Retry connection |
| **VCI Report Transmission** | 3 seconds | Retry transmission |

---

## Implementation Example

### Zonal Gateway (C Code)

```c
/* VCI Collection State */
typedef enum {
    VCI_STATE_IDLE,
    VCI_STATE_WAIT_ECU,
    VCI_STATE_READY,
    VCI_STATE_SENT
} VCI_Collection_State;

/* VCI Database */
#define MAX_ZONE_ECUS 10
VCI_Info g_vci_database[MAX_ZONE_ECUS];
uint8 g_vci_count = 0;

/* Expected ECU list for Zone 1 */
const char* expected_ecus[] = {
    "ECU_011"  // Body Domain ECU #1
};
#define EXPECTED_ECU_COUNT 1

/* VCI Collection Handler (called from UDS stack) */
void OnUDS_VCI_Received(const char* ecu_id, const VCI_Info* vci)
{
    // Store VCI in database
    if (g_vci_count < MAX_ZONE_ECUS)
    {
        memcpy(&g_vci_database[g_vci_count], vci, sizeof(VCI_Info));
        g_vci_count++;
        
        // Check if all expected ECUs reported
        if (g_vci_count >= EXPECTED_ECU_COUNT)
        {
            // Add Zonal Gateway VCI
            VCI_Info zgw_vci = {
                .ecu_id = "ECU_091",
                .sw_version = "0.0.0",
                .hw_version = "0.0.0",
                .serial_num = "091000001"
            };
            memcpy(&g_vci_database[g_vci_count], &zgw_vci, sizeof(VCI_Info));
            g_vci_count++;
            
            // Send to VMG via DoIP
            SendConsolidatedVCI_ToVMG(g_vci_database, g_vci_count);
        }
    }
}
```

---

## Testing Procedure

### Step 1: Zone ECU Simulation
```python
# Simulate ECU_011 sending VCI via UDS (CAN)
can_send(
    id=0x7E1,  # UDS Request to Zonal Gateway
    data=[0x2F, 0xF1, 0x87, 0x03,  # Service + DID + Control
          # ECU_011 VCI data (48 bytes)
          0x45, 0x43, 0x55, 0x5F, 0x30, 0x31, 0x31, ...  # "ECU_011"
    ]
)
```

### Step 2: Zonal Gateway VCI Collection
- Monitor UART log for VCI reception
- Verify VCI database population

### Step 3: VMG Report Verification
- Capture DoIP message from Zonal Gateway
- Verify consolidated VCI (ECU_091 + ECU_011)

---

## Error Handling

### Scenarios

| Error | Detection | Recovery |
|-------|-----------|----------|
| **ECU VCI Timeout** | No VCI received in 5s | Send partial VCI report |
| **Duplicate VCI** | Same ECU ID received twice | Keep latest VCI |
| **Invalid VCI Format** | Parsing error | Discard and log error |
| **DoIP Send Failure** | Transmission timeout | Retry 3 times |

---

## Future Extensions

### Multi-Zone Support
```
Zone 1 (Body):   ECU_011, ECU_012, ECU_013, ...
Zone 2 (ADAS):   ECU_021, ECU_022, ECU_023, ...
Zone 3 (Telem):  ECU_031, ECU_032, ECU_033, ...

Zonal Gateway:   ECU_091
```

### OTA Update Integration
- Update SW version after OTA
- Trigger VCI re-collection
- Report updated VCI to VMG

### Security Features
- VCI authentication via digital signature
- Encrypted VCI transmission
- Tamper detection

---

## Revision History

| Version | Date | Author | Changes |
|---------|------|--------|---------|
| 2.0 | 2025-11-04 | Zonal Gateway Team | Changed to Collect VCI Protocol |
| 1.0 | 2025-11-04 | Zonal Gateway Team | Initial VCI naming convention |

---

## References

- ISO 14229 (UDS - Unified Diagnostic Services)
- ISO 13400 (DoIP - Diagnostics over Internet Protocol)
- AUTOSAR Adaptive Platform Specification
- SAE J1939 (CAN-based communication)
