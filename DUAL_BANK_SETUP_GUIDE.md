# Dual Bank Flash Configuration Guide

## 📋 Overview

이 가이드는 ZGW의 안전한 OTA 업데이트를 위한 **듀얼 뱅크 Flash 구성**을 설명합니다.

---

## 🏗️ Architecture

### **Memory Layout**

```
TC375 PFLASH (Total 6MB):
┌─────────────────────────────────────────────────┐
│ Bank A (0xA0000000 - 0xA01FFFFF, 2MB)          │
│   ├─ 0xA0000000: Reset Vector                  │
│   ├─ 0xA0000100: SW Metadata (256 bytes)       │
│   └─ 0xA0000200: Application Code              │
├─────────────────────────────────────────────────┤
│ Bank B (0xA0200000 - 0xA03FFFFF, 2MB)          │
│   ├─ 0xA0200000: Reset Vector                  │
│   ├─ 0xA0200100: SW Metadata (256 bytes)       │
│   └─ 0xA0200200: Application Code              │
├─────────────────────────────────────────────────┤
│ Reserved (0xA0400000 - 0xA05FFFFF, 2MB)        │
│   └─ Future use / Multi-core CPU1/CPU2         │
└─────────────────────────────────────────────────┘
```

### **Dual Bank OTA Flow**

```
┌─────────────────────────────────────────────────┐
│ Boot: Bank A (v1.0.0)                           │
│   → Read Marker: 0xAAAAAAAA                    │
│   → Execute from 0xA0000000                     │
└─────────────────────────────────────────────────┘
        ↓
┌─────────────────────────────────────────────────┐
│ OTA: VMG sends v2.0.0                           │
│   → SPI Flash (0x00000000)                      │
│   → Copy to Bank B (0xA0200000) ← Standby!      │
│   → Verify CRC32                                │
│   → Set Bootloader Flag                         │
└─────────────────────────────────────────────────┘
        ↓
┌─────────────────────────────────────────────────┐
│ Reboot: Bootloader                              │
│   → Switch Marker: 0xBBBBBBBB                   │
│   → Boot from Bank B (v2.0.0)                   │
└─────────────────────────────────────────────────┘
        ↓
┌─────────────────────────────────────────────────┐
│ Next OTA: v3.0.0                                │
│   → Copy to Bank A (0xA0000000) ← Standby!      │
│   → Ping-Pong forever...                        │
└─────────────────────────────────────────────────┘
```

---

## 🛠️ Build Configuration

### **Step 1: Create Two Build Configurations**

#### **AURIX Studio 설정**

1. **Project Explorer** → `Zonal_Gateway` 우클릭
2. **Build Configurations** → **Manage...**
3. **New...** 클릭:
   - **Name**: `TriCore Debug (Bank A)`
   - **Copy settings from**: `TriCore Debug (TASKING)`
4. 다시 **New...** 클릭:
   - **Name**: `TriCore Debug (Bank B)`
   - **Copy settings from**: `TriCore Debug (TASKING)`

### **Step 2: Configure Linker Scripts**

#### **Bank A Configuration**

1. **Project Properties** → **C/C++ Build** → **Settings**
2. **TASKING Linker** → **Script File**
3. **Browse...** → `Lcf_Tasking_BankA.lsl` 선택

#### **Bank B Configuration**

1. 위와 동일한 과정
2. **Script File** → `Lcf_Tasking_BankB.lsl` 선택

---

## 🔨 Building Both Banks

### **Option A: Manual Build (개발 중)**

```powershell
# Bank A 빌드
Project → Build Configurations → Set Active → TriCore Debug (Bank A)
Project → Build Project

# Bank B 빌드
Project → Build Configurations → Set Active → TriCore Debug (Bank B)
Project → Build Project
```

**결과:**
- `TriCore Debug (Bank A)/Zonal_Gateway_BankA.elf`
- `TriCore Debug (Bank B)/Zonal_Gateway_BankB.elf`

### **Option B: Batch Build (권장)**

```powershell
# AURIX Studio
Project → Build Configurations → Build All
```

---

## 📥 Flashing Both Banks

### **Step 1: Flash Bank A**

1. **Debug Configuration** 생성:
   - **Name**: `Debug (Bank A)`
   - **C/C++ Application**: `TriCore Debug (Bank A)/Zonal_Gateway_BankA.elf`

2. **Run** → **Debug Configurations...**
3. **AURIX Second Generation - Default Debug** 선택
4. **Debug** 클릭

### **Step 2: Flash Bank B (동일한 코드)**

1. **Debug Configuration** 생성:
   - **Name**: `Debug (Bank B)`
   - **C/C++ Application**: `TriCore Debug (Bank B)/Zonal_Gateway_BankB.elf`

2. **Debug** 실행

---

## ✅ Verification

### **UART Output 확인**

```
*** BOOT INFORMATION ***
Reset Vector: 0xA0000000
Current PC:   0xA0000234
Running from: Bank A ✓
************************

[MCU Bank] Bank A is ACTIVE (0xA0000000)
[MCU Bank] Bank B is STANDBY (0xA0200000)
[VCI] SW Version: 1.0.0 (from Bank A metadata)
```

### **Memory Verify (via Debugger)**

```
Bank A (0xA0000000):
  → Reset Vector: 0xA0000000
  → First Instruction: Non-zero

Bank B (0xA0200000):
  → Reset Vector: 0xA0200000
  → First Instruction: Non-zero (same as Bank A)
```

---

## 🧪 OTA Testing

### **Test Scenario 1: Bank A → Bank B Update**

```python
# Python OTA Test
import socket

# 1. Connect to ZGW
sock = socket.socket(socket.AF_INET, socket.SOCK_TCP)
sock.connect(('192.168.1.10', 13400))

# 2. Send SW Package (v2.0.0)
#    → ZGW detects: Running from Bank A
#    → Writes to: Bank B (0xA0200000)

# 3. Reboot ZGW
#    → Bootloader switches to Bank B
#    → New UART output: "Running from: Bank B ✓"
```

### **Test Scenario 2: Rollback Test**

```
1. Update from Bank A (v1.0.0) to Bank B (v2.0.0)
2. New SW has bug → Manual reboot
3. Bootloader detects error → Rollback to Bank A
4. Safe fallback to v1.0.0 ✓
```

---

## 📊 Comparison: Bank A vs Bank B

| Feature | Bank A (0xA0000000) | Bank B (0xA0200000) |
|---------|---------------------|---------------------|
| Size | 2MB | 2MB |
| Reset Vector | 0xA0000000 | 0xA0200000 |
| Trap Vector | 0x80000100 | 0x80200100 |
| Interrupt Vector | 0x802FE000 | 0x804FE000 |
| SW Metadata | 0xA0000100 | 0xA0200100 |
| Application Code | 0xA0000200+ | 0xA0200200+ |
| Linker Script | `Lcf_Tasking_BankA.lsl` | `Lcf_Tasking_BankB.lsl` |
| Use Case | Current Stable SW | OTA Update Target |

---

## 🚨 Important Notes

### **⚠️ Never Erase Both Banks!**
- 최소 한 쪽 Bank는 항상 유효한 코드 보유
- OTA 실패 시 Rollback 보장

### **⚠️ Metadata Synchronization**
- 각 Bank의 Metadata (0xA0x00100)에 버전 정보 저장
- VCI 읽기 시 Active Bank의 Metadata 사용

### **⚠️ Bootloader Requirements**
- Bank Marker (DFLASH) 읽기/쓰기
- CRC32 검증 (선택사항, 빠른 부팅)
- Bank 전환 로직

---

## 🎯 Next Steps

1. ✅ **Linker Script 구성 완료** (Bank A/B)
2. ⏳ **두 뱅크 모두 빌드 & 플래시**
3. ⏳ **UART 출력으로 Active Bank 확인**
4. ⏳ **OTA 테스트 (Bank A → Bank B)**
5. ⏳ **Bootloader 구현** (Bank 전환)

---

## 📝 Build Summary

```bash
# Current Files
├── Lcf_Tasking_Tricore_Tc.lsl    # Original (deprecated)
├── Lcf_Tasking_BankA.lsl         # ✓ Bank A (0xA0000000)
└── Lcf_Tasking_BankB.lsl         # ✓ Bank B (0xA0200000)

# Build Outputs
├── TriCore Debug (Bank A)/
│   └── Zonal_Gateway_BankA.elf   # Flash to 0xA0000000
└── TriCore Debug (Bank B)/
    └── Zonal_Gateway_BankB.elf   # Flash to 0xA0200000
```

---

## ✅ Checklist

- [ ] AURIX Studio에서 두 Build Configuration 생성
- [ ] Bank A 빌드 성공
- [ ] Bank B 빌드 성공
- [ ] Bank A 플래시 완료
- [ ] Bank B 플래시 완료
- [ ] UART로 Active Bank 확인
- [ ] OTA 테스트 (SPI Flash → MCU PFLASH)
- [ ] Bootloader 플래그 확인
- [ ] Bank 전환 테스트

**모든 체크리스트 완료 → 안전한 OTA 시스템 완성!** 🎉

