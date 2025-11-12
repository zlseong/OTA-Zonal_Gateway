# VCI Collection System - 사용 가이드

## 📋 개요

Zonal Gateway의 VCI (Vehicle Configuration Information) 수집 시스템은 **명령 기반**으로 동작합니다.
VMG가 UDS 명령을 보내면 ZGW가 Zone ECU들에게 UDP 브로드캐스트로 VCI를 요청하고, 10초 타임아웃 내에 응답을 수집합니다.

## 🔄 동작 흐름

### 1단계: VCI 수집 시작
```
VMG → ZGW: UDS 0x31 01 F001 (Start VCI Collection)
         ↓
    ZGW: VCI 데이터베이스 초기화 (ZGW 정보만 남김)
         ↓
    ZGW → 브로드캐스트(192.168.1.255:13400): "RQST" (4 bytes)
         ↓
Zone ECU들: VCI 요청 수신 → VCI 응답 전송
         ↓
    ZGW: 10초 타임아웃 동안 VCI 수집
```

### 2단계: VCI 패키지 전송
```
VMG → ZGW: UDS 0x31 01 F002 (Send VCI Report)
         ↓
    ZGW → VMG: 수집된 VCI 패키지 (DoIP 메시지)
```

## 📦 UDS 명령 형식

### 명령 1: VCI 수집 시작 (0x31 01 F001)

**Request:**
```
31 01 F0 01
^^service ID (Routine Control)
   ^^sub-function (Start Routine)
      ^^^^routine ID (VCI Collection Start)
```

**Response:**
```
71 01 F0 01 00
^^service ID + 0x40 (Positive Response)
   ^^sub-function echo
      ^^^^routine ID echo
            ^^status (00=success)
```

### 명령 2: VCI 패키지 전송 (0x31 01 F002)

**Request:**
```
31 01 F0 02
^^service ID (Routine Control)
   ^^sub-function (Start Routine)
      ^^^^routine ID (VCI Send Report)
```

**Response:**
```
71 01 F0 02 00 02
^^service ID + 0x40
   ^^sub-function echo
      ^^^^routine ID echo
            ^^status (00=success, 01=not connected, 02=send error)
               ^^ECU count (예: 2개)
```

## 🌐 UDP 브로드캐스트 프로토콜

### VCI 요청 (ZGW → Zone ECUs)
- **목적지**: 192.168.1.255:13400
- **데이터**: `52 51 53 54` ("RQST")
- **길이**: 4 bytes

### VCI 응답 (Zone ECU → ZGW)
- **목적지**: 192.168.1.10:13400
- **데이터**: 
  ```
  [Magic: 4B][ECU ID: 16B][SW Ver: 8B][HW Ver: 8B][Serial: 16B]
  ```
- **길이**: 52 bytes
- **Magic**: `56 43 49 21` ("VCI!")

## ⏱️ 타임아웃 및 타이밍

- **VCI 수집 타임아웃**: 10초
  - VMG가 "VCI 수집 시작" 명령을 보낸 후
  - ZGW는 10초 동안 Zone ECU들의 VCI 응답을 대기
  - 10초 후 자동으로 수집 완료 처리
  
- **Zone ECU 응답 지연**: ~100ms
  - ECU는 요청 수신 후 즉시 응답 (시뮬레이션에서는 100ms 지연)

## 🧪 테스트 방법

### 1. Zone ECU 시뮬레이터 실행
```bash
cd Zonal_Gateway
python ecu_011_simulator.py
```

출력 예시:
```
============================================================
ECU_011 Simulator Started
Mode: Listen for VCI requests on UDP 13400
Press Ctrl+C to stop
============================================================
[ECU_011] Listening on UDP port 13400
[ECU_011] Waiting for VCI collection requests...
```

### 2. Zonal Gateway 실행
- AURIX TC375에 펌웨어 플래시
- UART 로그 확인

### 3. VCI 명령 테스트 스크립트 실행
```bash
python test_vci_commands.py
```

테스트 시나리오:
1. ZGW에 DoIP 연결
2. **TEST 1**: VCI 수집 시작 명령 전송
   - ZGW가 UDP 브로드캐스트 전송
   - ECU_011이 VCI 응답
3. 3초 대기 (Zone ECU 응답 시간)
4. **TEST 2**: VCI 패키지 전송 명령
   - ZGW가 수집된 VCI를 VMG에 전송

## 📊 VCI 데이터베이스 구조

```c
DoIP_VCI_Info g_vci_database[MAX_ZONE_ECUS + 1];
```

- **인덱스 0 ~ (g_zone_ecu_count-1)**: Zone ECU들의 VCI
- **인덱스 g_zone_ecu_count**: Zonal Gateway 자신의 VCI

### VCI 정보 구조체
```c
typedef struct
{
    char ecu_id[16];      // "ECU_011", "ECU_091" 등
    char sw_version[8];   // "0.0.0"
    char hw_version[8];   // "0.0.0"
    char serial_num[16];  // "011000001"
} DoIP_VCI_Info;
```

## 🔍 디버그 로그 예시

### ZGW 측 (UART)
```
[UDS] RX: SID=0x31, SA=0x0E00, TA=0x0100, Len=3
[VCI] Collection started (10s timeout)
[VCI] Broadcast request sent (192.168.1.255:13400)
[VCI] Received from ECU_011 (1/1)
[VCI] Collection timeout (1 Zone ECUs + ZGW)
[UDS] VCI report sent (2 ECUs)
```

### ECU_011 시뮬레이터 측
```
[ECU_011] ✓ VCI Request received from 192.168.1.10:13400

[ECU_011] VCI Response sent to 192.168.1.10:13400
  ECU ID: ECU_011
  SW Version: 0.0.0
  HW Version: 0.0.0
  Serial Number: 011000001
```

### VMG 테스트 스크립트 측
```
[TEST 1] Start VCI Collection
✓ SUCCESS: VCI collection started
  Status code: 0x00

Waiting 3 seconds for Zone ECUs to send VCI via UDP...

[TEST 2] Send VCI Report
✓ SUCCESS: VCI report sent
  Status code: 0x00
  ECU count: 2
```

## 🚨 에러 처리

### VCI 전송 실패 (Status Codes)
- `0x00`: 성공
- `0x01`: DoIP 연결 안됨
- `0x02`: TCP 전송 오류

### 타임아웃 처리
- 10초 내에 응답한 ECU들만 수집
- 타임아웃 후 ZGW 자신의 VCI 추가
- `g_vci_collection_complete` 플래그 설정

## 📝 참고사항

1. **브로드캐스트 주소**: 반드시 `192.168.1.255` 사용
2. **포트 충돌**: ECU 시뮬레이터와 ZGW는 서로 다른 IP여야 함
3. **동시 수집**: 한 번에 하나의 VCI 수집만 가능
4. **재수집**: 언제든지 "VCI 수집 시작" 명령으로 재수집 가능

## 🔧 코드 위치

- **VCI 수집 로직**: `Zonal_Gateway/Cpu0_Main.c`
  - `VCI_StartCollection()`: 수집 시작
  - `VCI_SendCollectionRequest()`: 브로드캐스트 전송
  - `VCI_CheckCollectionTimeout()`: 타임아웃 체크
  
- **UDS 핸들러**: `Zonal_Gateway/Libraries/DoIP/uds_handler.c`
  - `UDS_Service_RoutineControl()`: 0x31 서비스 핸들러
  - `UDS_RID_VCI_COLLECTION_START`: 0xF001
  - `UDS_RID_VCI_SEND_REPORT`: 0xF002

- **테스트 도구**:
  - `test_vci_commands.py`: VMG 시뮬레이터
  - `ecu_011_simulator.py`: Zone ECU 시뮬레이터

