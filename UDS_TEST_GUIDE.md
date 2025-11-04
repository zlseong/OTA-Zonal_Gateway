# UDS 기반 DoIP 통신 테스트 가이드

## 📋 개요

TC375 Zonal Gateway와 VMG 시뮬레이터 간의 **UDS over DoIP** 통신을 테스트합니다.

---

## 🔧 시스템 구성

```
[VMG Simulator]  ←→  [TC375 Zonal Gateway]  ←→  [Zone ECU Simulator]
  (PC Python)         (AURIX TC375)              (PC Python)
  192.168.1.100       192.168.1.10               192.168.1.11
  Port: 13400         DoIP Client                Port: 13400
```

---

## 🚀 테스트 절차

### **Step 1: VMG 시뮬레이터 실행**

```powershell
cd C:\Users\user\AURIX-v1.10.24-workspace\Zonal_Gateway
python vmg_simulator.py
```

**예상 출력:**
```
============================================================
  VMG (Vehicle Master Gateway) Simulator
  DoIP Server for Zonal Gateway Testing
============================================================

[HH:MM:SS.mmm] 📡 VMG Simulator starting on 0.0.0.0:13400
[HH:MM:SS.mmm] ✅ VMG Simulator started successfully
[HH:MM:SS.mmm] 📡 Waiting for Zonal Gateway connection...

[HH:MM:SS.mmm] 📡 Keyboard Commands:
[HH:MM:SS.mmm] 📡   v - Request Consolidated VCI (DID 0xF195)
[HH:MM:SS.mmm] 📡   h - Request Health Status (DID 0xF1A0)
[HH:MM:SS.mmm] 📡   q - Quit
```

---

### **Step 2: TC375 빌드 및 플래시**

1. **AURIX Development Studio** 실행
2. **Project → Build Project** (`Ctrl+B`)
3. 빌드 성공 확인
4. **Run → Debug** (또는 Flash 도구 사용)

---

### **Step 3: DoIP 연결 확인**

TC375가 부팅되면 VMG 시뮬레이터에서 다음 로그 확인:

```
[HH:MM:SS.mmm] ✅ Zonal Gateway connected from 192.168.1.10:XXXXX
[HH:MM:SS.mmm] 🔍 ← Received: Routing Activation Request
[HH:MM:SS.mmm] 📡 Routing Activation Request: Source=0x0100, Type=0
[HH:MM:SS.mmm] 🔍 → Sent: Routing Activation Response
[HH:MM:SS.mmm] ✅ Routing Activation successful - starting Alive Check
```

---

### **Step 4: UDS VCI 요청 테스트**

VMG 시뮬레이터 콘솔에서 **`v`** 입력 후 Enter:

```
v
```

**예상 로그:**
```
============================================================
[HH:MM:SS.mmm] ✅ UDS REQUEST SENT: Consolidated VCI (DID 0xf195)
[HH:MM:SS.mmm] 📡   VMG (0x0100) → ZGW (0x0100)
[HH:MM:SS.mmm] 📡   UDS: 22 f195
============================================================
```

TC375 콘솔(UART)에서 확인:
```
[UDS] RX: SID=0x22, SA=0x0100, TA=0x0100
[UDS 0x22] Read DID: 0xF195
[UDS 0x22] Responding with Consolidated VCI (0xF195)
```

---

### **Step 5: UDS Health Status 요청 테스트**

VMG 시뮬레이터 콘솔에서 **`h`** 입력 후 Enter:

```
h
```

**예상 로그:**
```
============================================================
[HH:MM:SS.mmm] ✅ UDS REQUEST SENT: Health Status (DID 0xf1a0)
[HH:MM:SS.mmm] 📡   VMG (0x0100) → ZGW (0x0100)
[HH:MM:SS.mmm] 📡   UDS: 22 f1a0
============================================================
```

TC375 콘솔(UART)에서 확인:
```
[UDS] RX: SID=0x22, SA=0x0100, TA=0x0100
[UDS 0x22] Read DID: 0xF1A0
[UDS 0x22] Responding with Health Status (0xF1A0)
```

---

## 📊 테스트 시나리오

### **시나리오 1: VCI 수집 (Request-Response Model)**

```
1. [VMG] 키보드 입력 'v' 또는 자동 트리거
2. [VMG] → [ZGW]: UDS 0x22 DID 0xF195 (Consolidated VCI 요청)
3. [ZGW]: 내부 VCI 데이터베이스 확인
4. [ZGW] → [VMG]: UDS 0x62 DID 0xF195 + VCI Data (응답)
5. [VMG]: VCI 정보 출력
```

---

### **시나리오 2: Health Status 모니터링**

```
1. [VMG] 키보드 입력 'h' 또는 주기적 트리거 (10초마다)
2. [VMG] → [ZGW]: UDS 0x22 DID 0xF1A0 (Health Status 요청)
3. [ZGW]: 현재 Health Status 수집
4. [ZGW] → [VMG]: UDS 0x62 DID 0xF1A0 + Health Data (응답)
5. [VMG]: Health 정보 출력
```

---

### **시나리오 3: Zone ECU와의 VCI 수집 (향후 구현)**

```
1. [VMG] → [ZGW]: UDS 0x22 DID 0xF195
2. [ZGW] → [Zone ECU]: UDS 0x22 DID 0xF194 (Individual VCI)
3. [Zone ECU] → [ZGW]: UDS 0x62 DID 0xF194 + VCI
4. [ZGW]: VCI 통합 (Zone ECU + 자신의 VCI)
5. [ZGW] → [VMG]: UDS 0x62 DID 0xF195 + Consolidated VCI
```

---

## 🔍 디버깅

### **TC375 UART 로그 확인**

- **Baud Rate**: 115200
- **Data**: 8 bit
- **Parity**: None
- **Stop Bit**: 1

**주요 로그 패턴:**
```
[DoIP Client] Init
[DoIP Client] Connecting to VMG...
[DoIP Client] Connected
[DoIP Client] Routing Activation sent
[DoIP Client] Active - ready for UDS
[UDS] RX: SID=0x22, SA=0x0100, TA=0x0100
[UDS 0x22] Read DID: 0xF195
```

---

### **일반적인 문제 해결**

#### **1. VMG 연결 실패**
- **증상**: `[DoIP Client] tcp_connect failed`
- **원인**: VMG 시뮬레이터 미실행 또는 IP 주소 불일치
- **해결**: 
  1. VMG 시뮬레이터 실행 확인
  2. IP 주소 확인 (TC375: 192.168.1.10, VMG: 192.168.1.100)
  3. 방화벽 확인

#### **2. Routing Activation 실패**
- **증상**: `[DoIP Client] Routing Activation timeout`
- **원인**: VMG 응답 지연 또는 네트워크 문제
- **해결**:
  1. 네트워크 케이블 확인
  2. Ping 테스트: `ping 192.168.1.100`
  3. VMG 시뮬레이터 재시작

#### **3. UDS 요청 수신 안됨**
- **증상**: VMG에서 'v' 입력 후 응답 없음
- **원인**: Routing 미활성화 또는 연결 끊김
- **해결**:
  1. DoIP 연결 상태 확인
  2. TC375 재부팅
  3. VMG 시뮬레이터 재시작

---

## 📝 참고 자료

- **DoIP 프로토콜**: ISO 13400-2
- **UDS 프로토콜**: ISO 14229-1
- **VCI Collection Protocol**: `COLLECT_VCI_PROTOCOL.md`
- **DoIP Types**: `Libraries/DoIP/doip_types.h`
- **UDS Handler**: `Libraries/DoIP/uds_handler.h`

---

## 🔍 **메시지 Hex Dump 확인 방법**

### **VMG 시뮬레이터에서 확인**

최신 버전에서는 **자동으로 Hex Dump**가 출력됩니다:

```
============================================================
UDS REQUEST SENT: Consolidated VCI (DID 0xf195)
  VMG (0x0100) → ZGW (0x0100)
  UDS: 22 f195
============================================================
[HH:MM:SS.mmm] 🐛 → Sent: Diagnostic Message (15 bytes)
[HH:MM:SS.mmm] 🐛    Raw Data: 02 fd 80 01 00 00 00 07 01 00 01 00 22 f1 95
[HH:MM:SS.mmm] 🐛    Header:   02 fd 80 01 00 00 00 07
[HH:MM:SS.mmm] 🐛    Payload:  01 00 01 00 22 f1 95
```

**해석:**
- `02 fd` - DoIP Version (0x02 / 0xFD)
- `80 01` - Payload Type (0x8001 = Diagnostic Message)
- `00 00 00 07` - Payload Length (7 bytes)
- `01 00` - Source Address (0x0100 = VMG)
- `01 00` - Target Address (0x0100 = ZGW)
- `22` - UDS Service (0x22 = ReadDataByIdentifier)
- `f1 95` - DID (0xF195 = Consolidated VCI)

---

### **TC375 UART에서 확인**

TC375는 수신/송신 메시지를 다음과 같이 출력합니다:

#### **수신 메시지:**
```
[UDS] RX: SID=0x22, SA=0x0100, TA=0x0100, Len=2
[UDS] Data: F1 95
```

#### **송신 메시지:**
```
[UDS] TX: SID=0x62, SA=0x0100, TA=0x0100, Total=109 bytes
[UDS] TX Data: 02 FD 80 01 00 00 00 65 01 00 01 00 62 F1 95 02 ...
```

**해석:**
- `SID=0x62` - Positive Response (0x22 + 0x40)
- `F1 95` - DID Echo
- `02` - ECU Count
- `...` - VCI Data (48 bytes per ECU)

---

### **Wireshark로 패킷 캡처** (고급)

더 상세한 분석이 필요하면 Wireshark 사용:

1. **Wireshark 설치 및 실행**
2. **캡처 인터페이스 선택** (TC375 연결된 어댑터)
3. **필터 입력**: `tcp.port == 13400`
4. **캡처 시작**

**DoIP 패킷 확인:**
```
Frame 1: 15 bytes on wire
Ethernet II, Src: PC_MAC, Dst: TC375_MAC
Internet Protocol Version 4, Src: 192.168.1.100, Dst: 192.168.1.10
Transmission Control Protocol, Src Port: 52371, Dst Port: 13400
Data (15 bytes)
    02 fd 80 01 00 00 00 07 01 00 01 00 22 f1 95
```

---

## 🎯 다음 단계

1. ✅ DoIP 연결 테스트
2. ✅ UDS 요청/응답 테스트
3. ⏳ Zone ECU 시뮬레이터 통합
4. ⏳ VCI 자동 수집 구현
5. ⏳ OTA 업데이트 구현 (UDS 0x34, 0x36, 0x37)

---

## 📞 문의

문제 발생 시:
1. TC375 UART 로그 캡처
2. VMG 시뮬레이터 로그 캡처
3. 네트워크 설정 확인 (`ipconfig /all`)
4. Wireshark로 DoIP 패킷 캡처 (Port 13400)

---

**작성일**: 2025-11-04  
**버전**: 1.0 (UDS 기반 구조)

