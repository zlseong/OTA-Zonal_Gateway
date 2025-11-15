# Vehicle Configuration Information (VCI) Specification

**프로젝트**: OTA-Zonal_Gateway (TC375 Lite Kit)  
**문서 버전**: 1.0  
**최종 업데이트**: 2025-01-13  

---

## 📋 목차

1. [개요](#개요)
2. [Level 1: Vehicle-Level VCI](#level-1-vehicle-level-vci)
3. [Level 2: ECU-Level VCI](#level-2-ecu-level-vci)
4. [VCI Collection Flow](#vci-collection-flow)
5. [UDS 접근 방법](#uds-접근-방법)
6. [데이터 정합성](#데이터-정합성)

---

## 개요

### VCI (Vehicle Configuration Information)란?

VCI는 차량 및 개별 ECU의 **식별 정보**, **버전 정보**, **상태 정보**를 포함하는 메타데이터입니다.

### 계층 구조

```
┌─────────────────────────────────────────────────────────────┐
│  Level 1: Vehicle-Level VCI                                 │
│  ├─ 저장 위치: ZGW DFlash (0xAF001000)                      │
│  ├─ 크기: 150 bytes                                         │
│  ├─ 생성 시점: 제조/서비스                                   │
│  └─ 목적: 차량 전체 식별 및 OTA 적합성 판단                  │
└─────────────────────────────────────────────────────────────┘
              ↓
┌─────────────────────────────────────────────────────────────┐
│  Level 2: ECU-Level VCI                                     │
│  ├─ 저장 위치: 각 ECU DFlash                                │
│  ├─ 크기: 48 bytes per ECU                                  │
│  ├─ 수집 방법: UDP Broadcast (ZGW → Zone ECUs)              │
│  └─ 목적: ECU별 버전 관리 및 업데이트 대상 식별              │
└─────────────────────────────────────────────────────────────┘
```

### 용도

| 단계 | VCI 사용 |
|------|----------|
| **제조** | Vehicle VCI 기록 (VIN, Model, Manufacturing Date) |
| **OTA 준비** | ZGW가 Zone ECU VCI 수집 → VMG 전송 |
| **OTA 적합성** | VMG가 Vehicle VCI와 Package 메타데이터 비교 |
| **업데이트** | ECU별 현재 버전 vs 패키지 버전 비교 |
| **진단** | UDS 0x22로 VCI 읽기 (정비소, 고객지원) |

---

## Level 1: Vehicle-Level VCI

### 개념

차량 전체의 식별 및 구성 정보로, **OTA 패키지의 적합성 판단**에 사용됩니다.

### 데이터 구조

```c
/**
 * \brief Vehicle-Level Configuration Information
 * 
 * ISO 14229 (UDS) 및 ISO 15765 (CAN) 표준 기반
 * Storage: ZGW DFlash (0xAF001000), 150 bytes
 */
typedef struct
{
    /* Vehicle Identification */
    char vin[18];                    /* VIN: 17자리 + NULL (e.g., "KMHXX00XXXX000000") */
    char vehicle_model[32];          /* Model Name (e.g., "Genesis GV80") */
    uint16 model_year;               /* Model Year (e.g., 2024) */
    char manufacturing_date[11];     /* Manufacturing Date (YYYY-MM-DD) */
    
    /* Vehicle Configuration */
    uint8 vehicle_type;              /* 1=Sedan, 2=SUV, 3=Truck, 4=EV, etc */
    uint8 drivetrain_type;           /* 1=FWD, 2=RWD, 3=AWD, 4=4WD */
    uint8 fuel_type;                 /* 1=Gasoline, 2=Diesel, 3=Electric, 4=Hybrid */
    uint16 engine_displacement_cc;   /* Engine Size (cc) - 0 for EV */
    uint16 battery_capacity_kwh;     /* Battery Capacity (kWh * 10) - 0 for ICE */
    
    /* Vehicle Status */
    uint32 total_mileage_km;         /* Total Odometer Reading (km) */
    uint32 total_operating_hours;    /* Total Operating Hours */
    uint16 number_of_ecus;           /* Total Number of ECUs in vehicle */
    uint8 e_e_architecture_version;  /* E/E Architecture Version */
    
    /* Regional Configuration */
    uint8 region_code;               /* 1=NA, 2=EU, 3=Asia, 4=China, etc */
    uint8 language;                  /* 1=EN, 2=DE, 3=KO, 4=ZH, etc */
    char homologation[16];           /* Homologation ID (EU Type Approval, etc) */
    
    /* OTA/SW Management */
    char master_sw_version[16];      /* Master SW Version (entire vehicle) */
    uint32 last_update_timestamp;    /* Last OTA Update (Unix timestamp) */
    uint8 update_history_count;      /* Number of OTA updates performed */
    
    /* Network Topology */
    uint8 network_architecture;      /* 1=Centralized, 2=Zonal, 3=Domain */
    uint8 number_of_zones;           /* Number of zones (for zonal arch) */
    uint8 number_of_domains;         /* Number of domains (for domain arch) */
    
} Vehicle_VCI;

/* Total size: ~150 bytes */
```

---

### 필드별 상세 설명

#### 🔹 Vehicle Identification

| 필드 | 타입 | 설명 | 예시 |
|------|------|------|------|
| `vin` | char[18] | **VIN (Vehicle Identification Number)**: ISO 3779 표준, 17자리 고유 번호 | `"KMHXX00XXXX000000"` |
| `vehicle_model` | char[32] | 차량 모델명 | `"Genesis GV80"` |
| `model_year` | uint16 | 모델 연도 | `2024` |
| `manufacturing_date` | char[11] | 제조일자 (ISO 8601) | `"2024-03-15"` |

#### 🔹 Vehicle Configuration

| 필드 | 타입 | 설명 | 가능한 값 |
|------|------|------|-----------|
| `vehicle_type` | uint8 | 차량 타입 | 1=Sedan, 2=SUV, 3=Truck, 4=EV |
| `drivetrain_type` | uint8 | 구동 방식 | 1=FWD, 2=RWD, 3=AWD, 4=4WD |
| `fuel_type` | uint8 | 연료 타입 | 1=Gasoline, 2=Diesel, 3=Electric, 4=Hybrid |
| `engine_displacement_cc` | uint16 | 배기량 (cc) | 2000 (EV는 0) |
| `battery_capacity_kwh` | uint16 | 배터리 용량 (kWh × 10) | 770 = 77.0kWh (ICE는 0) |

#### 🔹 Vehicle Status

| 필드 | 타입 | 설명 |
|------|------|------|
| `total_mileage_km` | uint32 | 총 주행거리 (km) |
| `total_operating_hours` | uint32 | 총 가동 시간 (hours) |
| `number_of_ecus` | uint16 | 차량 내 총 ECU 수 |
| `e_e_architecture_version` | uint8 | E/E 아키텍처 버전 |

#### 🔹 Regional Configuration

| 필드 | 타입 | 설명 | 가능한 값 |
|------|------|------|-----------|
| `region_code` | uint8 | 판매 지역 | 1=NA, 2=EU, 3=Asia, 4=China |
| `language` | uint8 | 기본 언어 | 1=EN, 2=DE, 3=KO, 4=ZH |
| `homologation` | char[16] | 형식 승인 번호 | EU Type Approval |

#### 🔹 OTA/SW Management

| 필드 | 타입 | 설명 |
|------|------|------|
| `master_sw_version` | char[16] | 차량 마스터 SW 버전 |
| `last_update_timestamp` | uint32 | 마지막 OTA 업데이트 시각 (Unix) |
| `update_history_count` | uint8 | OTA 업데이트 누적 횟수 |

#### 🔹 Network Topology

| 필드 | 타입 | 설명 | 가능한 값 |
|------|------|------|-----------|
| `network_architecture` | uint8 | 네트워크 아키텍처 | 1=Centralized, 2=Zonal, 3=Domain |
| `number_of_zones` | uint8 | Zone 수 (Zonal 아키텍처인 경우) | 1-7 |
| `number_of_domains` | uint8 | Domain 수 (Domain 아키텍처인 경우) | 1-10 |

---

### 저장 위치 및 접근

```
Storage:
  - Location: ZGW DFlash (0xAF001000)
  - Size: 150 bytes
  - Persistence: Non-volatile (survives power cycles)
  - Write Protection: Security Access required (UDS 0x27)

Access:
  - Manufacturing: Initial write during vehicle assembly
  - Service: Update via UDS 0x2E (WriteDataByIdentifier)
  - OTA: Read via UDS 0x22 (ReadDataByIdentifier)
  - Diagnostics: Read via OBD-II / DoIP
```

---

### 상수 정의

```c
/* Vehicle Type Definitions */
#define VEHICLE_TYPE_SEDAN          0x01
#define VEHICLE_TYPE_SUV            0x02
#define VEHICLE_TYPE_TRUCK          0x03
#define VEHICLE_TYPE_EV             0x04
#define VEHICLE_TYPE_PHEV           0x05
#define VEHICLE_TYPE_HEV            0x06

/* Drivetrain Type Definitions */
#define DRIVETRAIN_FWD              0x01  /* Front Wheel Drive */
#define DRIVETRAIN_RWD              0x02  /* Rear Wheel Drive */
#define DRIVETRAIN_AWD              0x03  /* All Wheel Drive */
#define DRIVETRAIN_4WD              0x04  /* 4 Wheel Drive */

/* Fuel Type Definitions */
#define FUEL_TYPE_GASOLINE          0x01
#define FUEL_TYPE_DIESEL            0x02
#define FUEL_TYPE_ELECTRIC          0x03
#define FUEL_TYPE_HYBRID            0x04
#define FUEL_TYPE_PHEV              0x05

/* Region Code Definitions */
#define REGION_CODE_NORTH_AMERICA   0x01  /* US, Canada, Mexico */
#define REGION_CODE_EUROPE          0x02  /* EU, UK, etc */
#define REGION_CODE_ASIA            0x03  /* Korea, Japan, etc */
#define REGION_CODE_CHINA           0x04  /* China specific */
#define REGION_CODE_MIDDLE_EAST     0x05  /* Middle East */

/* Language Definitions */
#define LANGUAGE_ENGLISH            0x01
#define LANGUAGE_GERMAN             0x02
#define LANGUAGE_KOREAN             0x03
#define LANGUAGE_CHINESE            0x04
#define LANGUAGE_JAPANESE           0x05

/* Network Architecture Definitions */
#define NETWORK_ARCH_CENTRALIZED    0x01  /* Single central gateway */
#define NETWORK_ARCH_ZONAL          0x02  /* Multiple zonal gateways */
#define NETWORK_ARCH_DOMAIN         0x03  /* Domain-based architecture */
```

---

## Level 2: ECU-Level VCI

### 개념

개별 ECU의 식별 및 버전 정보로, **OTA 업데이트 대상 선정** 및 **버전 관리**에 사용됩니다.

### 데이터 구조

```c
/**
 * \brief ECU-Level Configuration Information
 * 
 * Storage: Each ECU's DFlash, 48 bytes
 * Collection: UDP Broadcast/Unicast (Port 13400)
 */
typedef struct
{
    char ecu_id[16];        /* ECU ID (e.g., "ECU_091") */
    char sw_version[8];     /* Software version (e.g., "1.2.3") */
    char hw_version[8];     /* Hardware version (e.g., "2.0.1") */
    char serial_num[16];    /* Serial number (e.g., "091000001") */
} DoIP_VCI_Info;

/* Total size: 48 bytes per ECU */
```

---

### ECU ID 명명 규칙

ECU ID는 `"ECU_XYZ"` 형식을 따릅니다:

```
Format: ECU_XYZ

  X: ECU Category (1 digit)
    0 = Gateway (ZGW, CGW)
    1-7 = Zone ECUs (by zone number)
    8 = Body ECUs
    9 = ADAS ECUs
  
  Y: Zone Number (1 digit)
    0 = Central/Not applicable
    1-7 = Zone 1-7
    8-9 = Special zones
  
  Z: ECU Number within category (1 digit)
    0-9 = Incremental numbering
```

#### 예시

| ECU ID | 설명 |
|--------|------|
| `ECU_091` | Gateway, Zone 9 (central), ECU #1 → **ZGW** |
| `ECU_011` | Zone ECU, Zone 1, ECU #1 |
| `ECU_012` | Zone ECU, Zone 1, ECU #2 |
| `ECU_812` | Body ECU, Zone 1, ECU #2 (Door control) |
| `ECU_901` | ADAS ECU, Zone 0 (central), ECU #1 (Camera) |
| `ECU_922` | ADAS ECU, Zone 2, ECU #2 (Radar) |

---

### UDP 패킷 구조

#### Request (ZGW → Zone ECUs)

```
Protocol: UDP Broadcast
Destination: 192.168.1.255:13400
Payload: [0x52, 0x51, 0x53, 0x54]  // "RQST" magic number
Size: 4 bytes
```

#### Response (Zone ECU → ZGW)

```
Protocol: UDP Unicast
Destination: ZGW IP:13400
Payload:
  Offset 0-15:  ECU ID (16 bytes, null-terminated string)
  Offset 16-23: SW Version (8 bytes, null-terminated string)
  Offset 24-31: HW Version (8 bytes, null-terminated string)
  Offset 32-47: Serial Number (16 bytes, null-terminated string)
Total Size: 48 bytes
```

#### 예시 (Hex Dump)

```
Offset  00 01 02 03 04 05 06 07 08 09 0A 0B 0C 0D 0E 0F
------------------------------------------------------
0x00    45 43 55 5F 30 31 31 00 00 00 00 00 00 00 00 00  | ECU_011.........
0x10    31 2E 32 2E 33 00 00 00 32 2E 30 2E 31 00 00 00  | 1.2.3...2.0.1...
0x20    30 31 31 30 30 30 30 30 31 00 00 00 00 00 00 00  | 011000001.......
```

---

### 저장 및 관리

```
Storage:
  - Location: Each ECU's DFlash (local storage)
  - Size: 48 bytes
  - Persistence: Non-volatile
  
Collection:
  - Trigger: UDS 0x31 (Routine Control) RID 0xF001
  - Method: UDP Broadcast from ZGW
  - Response: UDP Unicast to ZGW
  - Timeout: 10 seconds
  
Usage:
  - OTA Pre-check: Collect all ECU versions
  - Update Target Selection: Compare current vs new versions
  - Post-OTA Verification: Confirm successful updates
```

---

## VCI Collection Flow

### 시퀀스 다이어그램

```
┌──────────┐       ┌──────────┐       ┌──────────┐
│   VMG    │       │   ZGW    │       │ Zone ECU │
│  Server  │       │          │       │          │
└────┬─────┘       └────┬─────┘       └────┬─────┘
     │                  │                  │
     │ ① UDS 0x31 F001  │                  │
     │ (CollectVCI)     │                  │
     ├─────────────────>│                  │
     │   DoIP/TCP       │                  │
     │                  │                  │
     │              ② UDP Broadcast        │
     │                  ├─────────────────>│
     │                  │  "RQST" (4 B)    │
     │                  │  192.168.1.255   │
     │                  │                  │
     │                  │  ③ UDP Unicast   │
     │                  │<─────────────────┤
     │                  │  VCI (48 bytes)  │
     │                  │  ECU IP:13400    │
     │                  │                  │
     │              ④ Store in database   │
     │                  ├─────────┐        │
     │                  │ g_vci_  │        │
     │                  │ database│        │
     │                  │<────────┘        │
     │                  │                  │
     │ ⑤ UDS 0x71 01    │                  │
     │ (Success)        │                  │
     │<─────────────────┤                  │
     │                  │                  │
     │ ⑥ UDS 0x22 F190  │                  │
     │ (ReadVCI)        │                  │
     ├─────────────────>│                  │
     │                  │                  │
     │ ⑦ VCI Data       │                  │
     │ (48*N bytes)     │                  │
     │<─────────────────┤                  │
     │                  │                  │
```

---

### 단계별 설명

#### ① VMG → ZGW: VCI 수집 요청
```c
/* UDS Request */
SID: 0x31 (RoutineControl)
Sub-function: 0x01 (Start)
RID: 0xF001 (CollectVCI)
```

#### ② ZGW → Zone ECUs: UDP Broadcast
```c
/* ZGW Implementation */
void VCI_StartCollection(void)
{
    uint8 request[4] = {'R', 'Q', 'S', 'T'};
    
    /* Clear previous database */
    memset(g_vci_database, 0, sizeof(g_vci_database));
    g_zone_ecu_count = 0;
    
    /* Send UDP broadcast */
    struct pbuf *p = pbuf_alloc(PBUF_TRANSPORT, 4, PBUF_RAM);
    memcpy(p->payload, request, 4);
    
    ip_addr_t broadcast_addr;
    IP4_ADDR(&broadcast_addr, 192, 168, 1, 255);
    
    udp_sendto(g_udp_server_pcb, p, &broadcast_addr, 13400);
    pbuf_free(p);
    
    sendUARTMessage("[VCI] Broadcast sent to 192.168.1.255:13400\r\n", 46);
}
```

#### ③ Zone ECUs → ZGW: UDP Unicast Response
```c
/* ECU Implementation (Simulated) */
void ECU_SendVCI(void)
{
    DoIP_VCI_Info vci = {
        .ecu_id = "ECU_011",
        .sw_version = "1.2.3",
        .hw_version = "2.0.1",
        .serial_num = "011000001"
    };
    
    /* Send UDP unicast to ZGW */
    udp_sendto(pcb, &vci, sizeof(vci), zgw_ip, 13400);
}
```

#### ④ ZGW: VCI 저장
```c
/* ZGW Storage */
DoIP_VCI_Info g_vci_database[MAX_ZONE_ECUS];
uint8 g_zone_ecu_count = 0;

void VCI_HandleResponse(DoIP_VCI_Info *vci)
{
    if (g_zone_ecu_count < MAX_ZONE_ECUS) {
        memcpy(&g_vci_database[g_zone_ecu_count], vci, sizeof(DoIP_VCI_Info));
        g_zone_ecu_count++;
        
        char log[64];
        sprintf(log, "[VCI] Received from %s (Total: %u)\r\n",
                vci->ecu_id, (unsigned int)g_zone_ecu_count);
        sendUARTMessage(log, strlen(log));
    }
}
```

#### ⑤ ZGW → VMG: 수집 완료 응답
```c
/* UDS Positive Response */
SID: 0x71 (Positive Response to 0x31)
Sub-function: 0x01
RID: 0xF001
Status: 0x00 (Success)
```

#### ⑥-⑦ VMG → ZGW: VCI 데이터 요청 및 응답
```c
/* UDS Request */
SID: 0x22 (ReadDataByIdentifier)
DID: 0xF190 (Consolidated VCI)

/* UDS Response */
SID: 0x62
DID: 0xF190
Data: [VCI_1 (48B)] [VCI_2 (48B)] ... [VCI_N (48B)]
Total: 48 * N bytes
```

---

## UDS 접근 방법

### Vehicle-Level VCI 접근

#### Read (UDS 0x22)

| DID | 데이터 | 크기 | 설명 |
|-----|--------|------|------|
| `0xF190` | VIN | 17 bytes | Vehicle Identification Number |
| `0xF191` | Vehicle Model | 32 bytes | Model name (e.g., "Genesis GV80") |
| `0xF192` | Total Mileage | 4 bytes | Odometer reading (km) |
| `0xF193` | Master SW Version | 16 bytes | Vehicle-level SW version |
| `0xF19F` | Full Vehicle VCI | 150 bytes | Complete Vehicle VCI structure |

#### Write (UDS 0x2E)

```c
/* Security Access Required */
SID: 0x27 (SecurityAccess)
Sub: 0x01 (RequestSeed)
  → Response: Seed (4 bytes)

SID: 0x27
Sub: 0x02 (SendKey)
Data: Key (4 bytes, calculated from seed)
  → Response: 0x67 02 (Success)

/* Write VCI */
SID: 0x2E (WriteDataByIdentifier)
DID: 0xF19F
Data: Vehicle_VCI (150 bytes)
  → Response: 0x6E F1 9F (Success)
```

---

### ECU-Level VCI 수집

#### Routine Control (UDS 0x31)

```c
/* Start VCI Collection */
Request:
  SID: 0x31 (RoutineControl)
  Sub: 0x01 (Start)
  RID: 0xF001 (CollectVCI)

Response:
  SID: 0x71 (Positive Response)
  Sub: 0x01
  RID: 0xF001
  Status: 0x00 (Success)
```

#### Read Collected VCI (UDS 0x22)

```c
/* Read All Collected VCI */
Request:
  SID: 0x22 (ReadDataByIdentifier)
  DID: 0xF190 (Consolidated VCI)

Response:
  SID: 0x62
  DID: 0xF190
  Data: [ECU_1_VCI (48B)] [ECU_2_VCI (48B)] ... [ECU_N_VCI (48B)]
  Total Size: 48 * N bytes
```

---

## 데이터 정합성

### Vehicle Package와 Vehicle VCI 매칭

OTA 패키지 적합성 검증 시, Vehicle Package Metadata의 다음 필드들이 Vehicle VCI와 **정확히 일치**해야 합니다:

```c
/* Vehicle_Package_Metadata (Level 3-1) */
typedef struct {
    /* Must match Vehicle VCI */
    char target_vin[18];             /* → Vehicle_VCI.vin */
    char target_vehicle_model[32];   /* → Vehicle_VCI.vehicle_model */
    uint16 target_model_year;        /* → Vehicle_VCI.model_year */
    uint8 required_architecture;     /* → Vehicle_VCI.network_architecture */
    uint8 required_num_zones;        /* → Vehicle_VCI.number_of_zones */
    
    /* Compatibility check */
    char min_vehicle_sw_version[16]; /* ≤ Vehicle_VCI.master_sw_version */
    uint8 target_regions[8];         /* Must include Vehicle_VCI.region_code */
} Vehicle_Package_Metadata;
```

---

### 검증 로직 (ZGW 구현)

```c
/**
 * \brief Validate OTA Package compatibility with Vehicle VCI
 * 
 * \param pkg Vehicle Package Metadata
 * \param vci Vehicle-Level VCI
 * \return TRUE if compatible, FALSE otherwise
 */
boolean Validate_PackageCompatibility(
    Vehicle_Package_Metadata *pkg,
    Vehicle_VCI *vci)
{
    /* 1. VIN 매칭 (정확히 일치 또는 wildcard) */
    if (strcmp(pkg->target_vin, "*") != 0) {
        if (strcmp(pkg->target_vin, vci->vin) != 0) {
            sendUARTMessage("[ERROR] VIN mismatch\r\n", 22);
            return FALSE;
        }
    }
    
    /* 2. Model 매칭 */
    if (strcmp(pkg->target_vehicle_model, vci->vehicle_model) != 0) {
        sendUARTMessage("[ERROR] Model mismatch\r\n", 24);
        return FALSE;
    }
    
    /* 3. Model Year 매칭 (0이면 any) */
    if (pkg->target_model_year != 0) {
        if (pkg->target_model_year != vci->model_year) {
            sendUARTMessage("[ERROR] Model year mismatch\r\n", 30);
            return FALSE;
        }
    }
    
    /* 4. Architecture 매칭 */
    if (pkg->required_architecture != vci->network_architecture) {
        sendUARTMessage("[ERROR] Architecture mismatch\r\n", 32);
        return FALSE;
    }
    
    /* 5. Zone 수 매칭 */
    if (pkg->required_num_zones != vci->number_of_zones) {
        sendUARTMessage("[ERROR] Zone count mismatch\r\n", 30);
        return FALSE;
    }
    
    /* 6. Region 매칭 */
    boolean region_match = FALSE;
    for (int i = 0; i < 8; i++) {
        if (pkg->target_regions[i] == vci->region_code) {
            region_match = TRUE;
            break;
        }
    }
    if (!region_match) {
        sendUARTMessage("[ERROR] Region not supported\r\n", 31);
        return FALSE;
    }
    
    /* 7. SW Version 호환성 (최소 버전 체크) */
    if (strcmp(pkg->min_vehicle_sw_version, vci->master_sw_version) > 0) {
        sendUARTMessage("[ERROR] SW version too old\r\n", 29);
        return FALSE;
    }
    
    sendUARTMessage("[OK] Package compatible with vehicle\r\n", 39);
    return TRUE;
}
```

---

### ECU VCI와 Zone Package 매칭

```c
/**
 * \brief Find target ECU in collected VCI database
 * 
 * \param target_ecu_id Target ECU ID from Zone Package
 * \param vci_db VCI database
 * \param count Number of ECUs in database
 * \return Pointer to ECU VCI, or NULL if not found
 */
DoIP_VCI_Info* Find_ECU_VCI(
    const char *target_ecu_id,
    DoIP_VCI_Info *vci_db,
    uint8 count)
{
    for (uint8 i = 0; i < count; i++) {
        if (strcmp(vci_db[i].ecu_id, target_ecu_id) == 0) {
            return &vci_db[i];
        }
    }
    return NULL;
}

/**
 * \brief Check if ECU needs update
 */
boolean Check_ECU_UpdateRequired(
    Zone_Package_Metadata *zpkg,
    DoIP_VCI_Info *ecu_vci)
{
    /* Compare current version with target version */
    if (strcmp(zpkg->ecu_packages[0].current_sw_version, 
               ecu_vci->sw_version) == 0) {
        /* Current version matches required version */
        return TRUE;
    } else {
        char log[128];
        sprintf(log, "[WARNING] ECU %s version mismatch: "
                     "current=%s, required=%s\r\n",
                ecu_vci->ecu_id,
                ecu_vci->sw_version,
                zpkg->ecu_packages[0].current_sw_version);
        sendUARTMessage(log, strlen(log));
        return FALSE;
    }
}
```

---

## 참조 문서

- **ISO 14229-1**: Unified Diagnostic Services (UDS)
- **ISO 15765-2**: Diagnostic communication over CAN (DoCAN)
- **ISO 13400**: Diagnostic communication over IP (DoIP)
- **ISO 3779**: Vehicle Identification Number (VIN)
- **memory_map.md**: AURIX TC375 메모리 맵
- **Metadata_information.md**: OTA 패키지 메타데이터 구조

---

**문서 종료**


