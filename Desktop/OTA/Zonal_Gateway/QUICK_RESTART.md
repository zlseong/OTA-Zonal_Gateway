# 🔄 빠른 재시작 가이드

## 문제: TC375 응답 없음

VMG가 UDS 요청을 보냈지만 TC375로부터 응답이 오지 않는 경우

---

## ✅ 해결 절차

### **1단계: VMG 시뮬레이터 종료**

```powershell
# VMG 콘솔에서 Ctrl+C 또는 'q' 입력
q
```

---

### **2단계: VMG 시뮬레이터 재시작**

```powershell
cd C:\Users\user\AURIX-v1.10.24-workspace\Zonal_Gateway
python vmg_simulator.py
```

**대기 메시지 확인:**
```
[시간] 📡 VMG Simulator started successfully
[시간] 📡 Waiting for Zonal Gateway connection...
```

---

### **3단계: TC375 리셋**

**옵션 A: 디버거에서 리셋**
- AURIX Development Studio에서 **Restart** 버튼 클릭

**옵션 B: 재플래시**
- **Run → Debug** 다시 실행

---

### **4단계: 연결 확인**

#### **VMG 콘솔에서 확인:**
```
[시간] ✅ Zonal Gateway connected from 192.168.1.10:XXXXX
[시간] 🔍 ← Received: Routing Activation Request
[시간] 📡 Routing Activation Request: Source=0x0100, Type=0
[시간] 🔍 → Sent: Routing Activation Response
[시간] ✅ Routing Activation successful - starting Alive Check
```

#### **TC375 UART에서 확인:**
```
[DoIP] TCP connected
[DoIP] Routing Activation Request sent
[DoIP] Routing Activation successful (Code: 0x10)
[DoIP] Client state: ACTIVE
```

---

### **5단계: UDS 요청 테스트**

VMG 콘솔에서:
```
v    # VCI 요청
```

**예상 응답:**
```
[시간] ✅ UDS REQUEST SENT: Consolidated VCI (DID 0xf195)
[시간] 🔍 ← Received: Diagnostic Message (109 bytes)
[시간] 📡 ============================================================
[시간] ✅ UDS Message Received from ZGW

  📊 VCI CONSOLIDATED DATA:
  Total ECUs: 2
  🔧 ECU #1:
     ECU ID:     ECU_011
     SW Version: 1.0.0
     ...
```

---

## 🐛 여전히 안 되는 경우

### **네트워크 확인:**

```powershell
# 1. TC375 Ping 테스트
ping 192.168.1.10

# 2. 포트 사용 확인
netstat -an | findstr "13400"

# 3. 방화벽 확인
# Windows Defender 방화벽 → 허용된 앱 → Python 확인
```

### **TC375 UART 로그 공유**

전체 부팅 로그를 캡처하여 공유:
```
PHY Link UP! Network ready.
[DoIP] Client initialized
...
```

---

## 📝 체크리스트

- [ ] VMG 먼저 시작
- [ ] "Waiting for Zonal Gateway connection..." 메시지 확인
- [ ] TC375 리셋/재실행
- [ ] VMG에서 "Zonal Gateway connected" 확인
- [ ] VMG에서 "Routing Activation successful" 확인
- [ ] TC375 UART에서 "Client state: ACTIVE" 확인
- [ ] VMG에서 `v` 입력
- [ ] 파싱된 VCI 데이터 표시 확인

---

**작성일**: 2025-11-04  
**버전**: 1.0

