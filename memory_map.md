# AURIX TC37x Memory Map Reference

**프로젝트**: OTA-Zonal_Gateway (TC375 Lite Kit)  
**문서 버전**: 2.0  
**최종 업데이트**: 2025-01-13 (문서 분리 및 구조 개선)  

---

## 📋 목차

### 핵심 메모리 맵
1. [PFlash (Program Flash)](#pflash-program-flash)
2. [DFlash (Data Flash)](#dflash-data-flash)
3. [UCB (User Configuration Block)](#ucb-user-configuration-block)
4. [RAM (DLMU, SRAM)](#ram-memory)
5. [Peripheral Registers](#peripheral-registers)

### OTA 관련 문서 (분리됨)
- **[VCI_information.md](VCI_information.md)**: Vehicle 및 ECU VCI 구조, 수집 방법
- **[Metadata_information.md](Metadata_information.md)**: OTA 패키지 메타데이터 계층 구조

---

## PFlash (Program Flash)

### 주소 범위 및 구조

```
┌─────────────────────────────────────────────────────────────┐
│ AURIX TC375 Flash Memory Layout                             │
├─────────────────────────────────────────────────────────────┤
│ Address Range        Size        Description                │
├─────────────────────────────────────────────────────────────┤
│ 0xA0000000          4 KB        🔧 Bootloader                │
│ ~ 0xA0000FFF                    - Single, Immutable         │
│                                 - DFlash 플래그 읽기         │
│                                 - Bank A/B 선택             │
│                                 - CRC 검증                  │
│                                 - Jump to Application        │
├─────────────────────────────────────────────────────────────┤
│ 0xA0001000          3,068 KB    📦 Application Bank A       │
│ ~ 0xA02FFFFF                    - Main Application          │
│                                 - DoIP/UDS/lwIP             │
│                                 - Flash Manager             │
│                                 - OTA Logic                 │
├─────────────────────────────────────────────────────────────┤
│ 0xA0300000          3,072 KB    📦 Application Bank B       │
│ ~ 0xA05FFFFF                    - OTA Update Target         │
│                                 - Rollback Backup           │
│                                 - SW Synchronization        │
└─────────────────────────────────────────────────────────────┘

Total: 6MB Program Flash (4KB Bootloader + 3068KB Bank A + 3072KB Bank B)
```

### 메모리 상세

| Component | Start Address | Size | Description |
|-----------|--------------|------|-------------|
| **Bootloader** | `0xA0000000` | 4 KB | 단일 고정 부트로더 |
| **Bank A** | `0xA0001000` | 3,068 KB | 활성 애플리케이션 (Bootloader 제외) |
| **Bank B** | `0xA0300000` | 3,072 KB | OTA 대상 뱅크 |

### 특징

- **읽기**: ok (Cached/Non-cached 접근 가능)
- **쓰기**: 프로그래밍 모드에서만 가능
- **지우기**: Sector 단위 (최소 8KB)
- **Dual Bank**: 안전한 OTA 업데이트 지원
- **Bootloader**: 4KB 예약 영역, DFlash 플래그 기반 Bank 선택

---

## DFlash (Data Flash)

| 주소 | 크기 | 용도 | 읽기 | 쓰기 |
|------|------|------|------|------|
| 0xAF400000 | - | UCB_BMHD0_ORIG (Boot Mode Header 0 원본) | ok | ok |
| 0xAF400200 | - | UCB_BMHD0_COPY (Boot Mode Header 0 백업) | ok | ok |
| 0xAF400400 | - | UCB_BMHD1_ORIG (Boot Mode Header 1 원본) | ok | ok |
| 0xAF400600 | - | UCB_BMHD1_COPY (Boot Mode Header 1 백업) | ok | ok |
| 0xAF400800 | - | UCB_BMHD2_ORIG (Boot Mode Header 2 원본) | ok | ok |
| 0xAF400A00 | - | UCB_BMHD2_COPY (Boot Mode Header 2 백업) | ok | ok |
| 0xAF400C00 | - | UCB_BMHD3_ORIG (Boot Mode Header 3 원본) | ok | ok |
| 0xAF400E00 | - | UCB_BMHD3_COPY (Boot Mode Header 3 백업) | ok | ok |
| - | - | UCB_SSW (Startup Software) | ok | ok |
| - | - | UCB_USER (사용자 정의) | ok | ok |
| - | - | UCB_TEST (테스트) | ok | ok |
| - | - | UCB_HSMCFG (HSM 설정) | ok | ok |
| - | - | UCB_REDSEC (Redundancy Sector) | ok | ok |
| - | - | UCB_RETEST | ok | ok |
| - | - | UCB_PFLASH_ORIG (PFlash 보호 설정 원본) | ok | ok |
| - | - | UCB_PFLASH_COPY (PFlash 보호 설정 백업) | ok | ok |
| - | - | UCB_DFLASH_ORIG (DFlash 보호 설정 원본) | ok | ok |
| - | - | UCB_DFLASH_COPY (DFlash 보호 설정 백업) | ok | ok |
| - | - | UCB_DBG_ORIG (디버그 설정 원본) | ok | ok |
| - | - | UCB_DBG_COPY (디버그 설정 백업) | ok | ok |
| - | - | UCB_HSM_ORIG (HSM 원본) | ok | ok |
| - | - | UCB_HSM_COPY (HSM 백업) | ok | ok |
| - | - | UCB_HSMCOTP0_ORIG | ok | ok |
| - | - | UCB_HSMCOTP0_COPY | ok | ok |
| - | - | UCB_HSMCOTP1_ORIG | ok | ok |
| - | - | UCB_HSMCOTP1_COPY | ok | ok |
| - | - | UCB_ECPRIO_ORIG | ok | ok |
| - | - | UCB_ECPRIO_COPY | ok | ok |
| - | - | UCB_SWAP_ORIG | ok | ok |
| - | - | UCB_SWAP_COPY | ok | ok |
| - | - | UCB_OTP0_ORIG ~ UCB_OTP7_ORIG (OTP 원본) | ok | ok |
| - | - | UCB_OPT1_COPY ~ UCB_OPT7_COPY (OTP 백업) | ok | ok |

### BMHD (Boot Mode Header) 구조

**Offset 0x000 ~ 0x00F: 헤더 정보**
```c
+0x000: uint16 bmi;          // Boot Mode Index (0x007F)
+0x002: uint16 bmhdid;       // Boot Mode Header ID (0xB359)
+0x004: uint32 stad;         // Start Address (부팅 주소) ← 뱅크 전환 시 변경!
+0x008: uint32 crc;          // CRC32 Checksum
+0x00C: uint32 crcInv;       // CRC32 Inverted
```

**Offset 0x010 ~ 0x0FF: Reserved**

**Offset 0x100 ~ 0x11F: Password Protection**
```c
+0x100: uint32 pw0 ~ pw7;    // 8개의 32비트 패스워드
```

**Offset 0x120 ~ 0x1EF: Reserved**

**Offset 0x1F0: Confirmation**
```c
+0x1F0: uint32 confirmation; // 항상 0x43211234
```

---

## DFlash (Data Flash)

### 주소 범위
- **Base**: `0xAF000000` ~ `0xAF03FFFF` (256 KB) ← **Corrected!**
- **Alternative Access**: `0xAFC00000` ~ `0xAFC1FFFF` (128 KB, EEPROM Emulation)
- **EEPROM 에뮬레이션**: DF1 (Data Flash 1)
- **HSM Command**: HSM Command Sequence Interpreter

### 특징
- **읽기**: ok
- **쓰기**: ok (Page 단위)
- **용도**: 
  - 설정 데이터 저장
  - Calibration 데이터
  - **OTA Boot Flag & Status (7-bit system)** ← **Primary Use!**
  - NVM (Non-Volatile Memory)

### DFlash 프로그래밍
- **Page 크기**: 32 bytes (일부 영역 512 bytes)
- **API**: `IfxFlash_enterPageMode()`, `IfxFlash_loadPage2X32()`

### OTA-Specific DFlash Usage
```c
// Boot Configuration Storage (8-bit Flash Bank Status)
#define DFLASH_BASE_ADDRESS         0xAF000000
#define DFLASH_BANK_STATUS_OFFSET   0x00000000

typedef union {
    uint8 U;
    struct {
        uint8 bootTarget:1;      // Bit 0: Boot Target (0=Bank A, 1=Bank B)
        uint8 statusA:2;         // Bit 1-2: Bank A Status (OK/UPDATING/ERROR)
        uint8 statusB:2;         // Bit 3-4: Bank B Status (OK/UPDATING/ERROR)
        uint8 banksIdentical:1;  // Bit 5: Banks Identical Flag (0=Different, 1=Same)
        uint8 criticalError:1;   // Bit 6: Critical Error Flag (0=OK, 1=Critical)
        uint8 syncInProgress:1;  // Bit 7: Synchronization in progress (0=No, 1=Yes)
    } bits;
} FlashBankStatus;

// Bank Status Values (2-bit encoding)
#define BANK_STATUS_OK          0x00  // 00: Bank is valid and operational
#define BANK_STATUS_UPDATING    0x01  // 01: OTA update in progress
#define BANK_STATUS_ERROR       0x02  // 10: Bank is corrupted or invalid
#define BANK_STATUS_RESERVED    0x03  // 11: Reserved for future use
```

---

## PFlash (Program Flash)

### TC37x PFlash Bank 구조 (Dual-Bank OTA Architecture)

```
┌─────────────────────────────────────────────────────────────┐
│                AURIX TC375 Flash Memory Layout              │
├─────────────────────────────────────────────────────────────┤
│ Address Range          Size        Description              │
├─────────────────────────────────────────────────────────────┤
│ 0xA0000000            4 KB       🔧 Bootloader              │
│   ~ 0xA0000FFF                   - Single, Immutable        │
│                                  - DFlash 플래그 읽기        │
│                                  - Bank A/B 선택             │
│                                  - CRC 검증                  │
│                                  - Jump to Application       │
├─────────────────────────────────────────────────────────────┤
│ 0xA0001000         3,068 KB      📦 Application Bank A      │
│   ~ 0xA02FFFFF                   - Main Application         │
│                                  - DoIP/UDS/lwIP            │
│                                  - Flash Manager            │
│                                  - OTA Logic                │
├─────────────────────────────────────────────────────────────┤
│ 0xA0300000         3,072 KB      📦 Application Bank B      │
│   ~ 0xA05FFFFF                   - OTA Update Target        │
│                                  - Rollback Backup          │
│                                  - SW Synchronization       │
├─────────────────────────────────────────────────────────────┤
│ 0xAF000000          256 KB       🗂️ DFlash (Data Flash)    │
│                                  - Boot Target Flag         │
│                                  - Bank Status              │
│                                  - CRC Values               │
├─────────────────────────────────────────────────────────────┤
│ 0xAF400000           24 KB       🔒 UCB (User Config Block) │
│                                  - BMHD (Boot Header)       │
│                                  - STAD = 0xA0000000        │
└─────────────────────────────────────────────────────────────┘

Total: 6MB Program Flash + 256KB Data Flash + 24KB UCB
```

### PFlash Detailed Memory Map

| Region | 주소 범위 (Physical) | 주소 범위 (Cached) | 크기 | 용도 |
|--------|---------------------|-------------------|------|------|
| **Bootloader** | 0xA0000000 ~ 0xA0000FFF | 0x80000000 ~ 0x80000FFF | 4 KB | Single Bootloader (Immutable) |
| **PFlash0 (Bank A)** | 0xA0001000 ~ 0xA02FFFFF | 0x80001000 ~ 0x802FFFFF | 3,068 KB | Application Bank A (Primary) |
| **PFlash1 (Bank B)** | 0xA0300000 ~ 0xA05FFFFF | 0x80300000 ~ 0x805FFFFF | 3,072 KB | Application Bank B (Backup/OTA) |
| **Total** | - | - | **6,144 KB** | Bootloader + Dual Applications |

### 주소 종류
1. **Physical (Non-cached)**: `0xA0000000` ~ 
   - Flash 프로그래밍 시 사용
   - 읽기: ok, 쓰기: ok (Flash 명령 사용)

2. **Cached**: `0x80000000` ~
   - 코드 실행 시 사용
   - 읽기: ok (빠름), 쓰기: 불가

### PFlash 프로그래밍
- **Sector 크기**: 256 KB (Logical Sector)
- **Page 크기**: 32 bytes
- **Wordline**: 1024 bytes (프로그래밍 단위)
- **API**: 
  - Erase: `IfxFlash_eraseSector()`
  - Program: `IfxFlash_enterPageMode()`, `IfxFlash_loadPage2X32()`
  - Wait: `IfxFlash_waitUnbusyAll()`

---

## RAM Memory

### DLMU (Data Local Memory)

| 주소 범위 | 크기 | 용도 | 읽기 | 쓰기 |
|----------|------|------|------|------|
| 0xB0000000 ~ 0xB000FFFF | 64 KB | DLMU RAM (CPU0_NC) | ok | ok |
| 0xB0010000 ~ 0xB001FFFF | 64 KB | DLMU RAM (CPU1_NC) | ok | ok |
| 0xB0020000 ~ 0xB002FFFF | 64 KB | DLMU RAM (CPU2_NC) | ok | ok |

### PSRAM (Program Scratch Pad RAM)
- **주소**: Cached 0xC0000000, Physical 0x70100000
- **크기**: 64 KB
- **용도**: 빠른 코드 실행

### DAM RAM
- **주소**: 0xB0400000 ~ 0xB0407FFF
- **크기**: 32 KB
- **특징**: Cached & Non-cached 접근 가능

---

## Peripheral Registers

### 중요 주변장치 주소

| 주소 범위 | 크기 | 모듈 | 읽기 | 쓰기 |
|----------|------|------|------|------|
| 0xF0000400 ~ 0xF00005FF | 512 B | FPI slave (CBS) | ok | ok |
| 0xF0000600 ~ 0xF00006FF | 256 B | FPI slave (ASCLIN0) | ok | ok |
| 0xF0000700 ~ 0xF00007FF | 256 B | FPI slave (ASCLIN1) | ok | ok |
| 0xF0000800 ~ 0xF00008FF | 256 B | FPI slave (ASCLIN2) | ok | ok |
| 0xF0001000 ~ 0xF00010FF | 256 B | STM0 (System Timer) | ok | ok |
| 0xF0001100 ~ 0xF00011FF | 256 B | STM1 | ok | ok |
| 0xF0001200 ~ 0xF00012FF | 256 B | STM2 | ok | ok |
| 0xF0001C00 ~ 0xF0001CFF | 256 B | QSPI0 | ok | ok |
| 0xF0001D00 ~ 0xF0001DFF | 256 B | QSPI1 | ok | ok |
| 0xF0001E00 ~ 0xF0001EFF | 256 B | QSPI2 | ok | ok |
| 0xF0001F00 ~ 0xF0001FFF | 256 B | QSPI3 | ok | ok |
| 0xF0002000 ~ 0xF00020FF | 256 B | QSPI4 | ok | ok |
| 0xF001D000 ~ 0xF001F0FF | 8.2 KB | GETH (Ethernet) | ok | ok |
| 0xF0030000 ~ 0xF00300FF | 256 B | SBCU Registers | ok | ok |
| 0xF0036000 ~ 0xF003636FF | 1 KB | SCU (System Control Unit) | ok | ok |
| 0xF0400000 ~ 0xF0405FFF | 128 KB | HSM (Hardware Security Module) | 32 | 32 |
| 0xF8040000 ~ 0xF807FFFF | 256 KB | DMU (Flash Memory Unit) | ok | ok |
| 0xF8800000 ~ 0xF881FFFF | 128 KB | CPU0 레지스터 | ok | ok |
| 0xF8820000 ~ 0xF883FFFF | 128 KB | CPU1 레지스터 | ok | ok |
| 0xF8840000 ~ 0xF885FFFF | 128 KB | CPU2 레지스터 | ok | ok |
| 0xFFC00000 ~ 0xFFC1FFFF | 128 KB | DFlash 1 EEPROM (DF1) | ok | ok |
| 0xFFFF0000 ~ 0xFFFFFFFF | 64 KB | Boot ROM (BROM) DMU | ok | ok |

### DMU (Flash Memory Unit) 레지스터
- **Base**: 0xF8040000
- **주요 레지스터**:
  - `DMU_HF_STATUS`: Flash 동작 상태
  - `DMU_HF_CCONTROL`: Cranking 모드 제어
  - `DMU_HF_PCONTROL`: Demand 모드 제어

### SCU (System Control Unit)
- **Base**: 0xF0036000
- **주요 기능**:
  - 클럭 설정
  - 전원 관리
  - 리셋 제어
  - **리셋 타입 감지**: `SCU_STMEM3` ~ `SCU_STMEM6` 레지스터로 Checker Software exit 정보 확인

### SCU 주요 레지스터 (OTA 관련)

| 레지스터 | 오프셋 | 접근 | Reset | 용도 |
|----------|--------|------|-------|------|
| SCU_STMEM1 | 0x0184 | U,SV | PowerOn Reset | Start-up Memory Register 1 |
| SCU_STMEM2 | 0x0188 | U,SV | System Reset | Start-up Memory Register 2 |
| **SCU_STMEM3** | **0x01C0** | **U,SV** | **Application Reset** | **Start-up Memory Register 3 (리셋 타입 감지)** |
| **SCU_STMEM4** | **0x01C4** | **U,SV** | **Cold PowerOn Reset** | **Start-up Memory Register 4** |
| **SCU_STMEM5** | **0x01C8** | **U,SV** | **PowerOn Reset** | **Start-up Memory Register 5** |
| **SCU_STMEM6** | **0x01CC** | **U,SV** | **System Reset** | **Start-up Memory Register 6** |

**접근 모드**: U,SV = User mode, Supervisor mode / ST,P0 = Supervisor Test mode, Protection 0

### SCU Reset Type 감지

| Reset Type | SCU_STMEM3 | SCU_STMEM4 | SCU_STMEM5 | SCU_STMEM6 |
|------------|------------|------------|------------|------------|
| Cold power-on | A030F81F | 00000001 | A030F81F | A030F81F |
| Warm power-on | A020F82F | 00000001 | A020F82F | A020F82F |
| System reset | 2020B84F | 00000001 | 2020B84F | 2020B84F |
| Application reset | 2020D88F | 00000001 | 2020088F | 2020088F |

**사용 예시**:
```c
// 리셋 타입 감지
uint32 stmem3 = SCU_STMEM3.U;  // 0xF0036000 + 0x01C0 = 0xF00361C0
if (stmem3 == 0x2020D88F) {
    // Application reset 발생 → OTA 롤백 시나리오 확인
}
```

**용도**: OTA 롤백 시 리셋 원인을 판별하여 부트 로직을 결정할 수 있음

### OLDA (Online Data Acquisition)
- **Base (Cached)**: 0x8FE00000
- **Base (Non-cached)**: 0xAFE00000
- **Size**: 512 KB (0x80000)
- **용도**: 
  - 실시간 데이터 수집
  - 디버그/진단 목적
  - OTA 진단 로그 저장 (선택적)

---

## OTA 듀얼뱅크 관련 메모리 영역

### Architecture Overview
```
Boot Sequence:
Boot ROM → UCB_BMHD (STAD) → Bootloader (0xA0000000) → Application (Bank A/B)
```

### 1. Boot 설정 (UCB)
```
0xAF400000: BMHD0_ORIG
  - .stad = 0xA0000000 (Bootloader 고정!)
  - CRC 포함 (필수)
  - ⚠️ 주의: STAD는 Bootloader 주소로 고정, 변경 권장 안함!
```

**Important**: UCB의 STAD는 `0xA0000000`으로 고정하고, Bootloader가 DFlash를 읽어 Bank A/B를 선택하는 방식 권장!

### 2. 상태 저장 (DFlash) - Software-Defined Boot Control
```
0xAF000000: FlashBankStatus (8-bit 플래그)
  - Bit 0: Boot Target (0=Bank A, 1=Bank B)
  - Bit 1-2: Bank A Status (00=OK, 01=UPDATING, 10=ERROR)
  - Bit 3-4: Bank B Status (00=OK, 01=UPDATING, 10=ERROR)
  - Bit 5: Banks Identical (0=Different, 1=Identical)
  - Bit 6: Critical Error (0=OK, 1=Critical)
  - Bit 7: Sync In Progress (0=No, 1=Yes)
```

**Bootloader Logic**:
```c
// Bootloader reads DFlash and decides which bank to boot
FlashBankStatus status = Read_DFlash();
uint32 app_addr = (status.bits.bootTarget == 0) 
                  ? 0xA0001000  // Bank A
                  : 0xA0300000; // Bank B

if (Verify_CRC(app_addr)) {
    Jump_To(app_addr);
} else {
    // Fallback to opposite bank
    Jump_To_Opposite_Bank();
}
```

**Application Init Logic (without Bootloader)**:
```c
// During SystemInit, check if banks are identical
FlashBankStatus status = Read_DFlash();

if (status.bits.banksIdentical == 0) {
    // Banks are different, check if they should be identical
    uint32 crc_A = Calculate_CRC32(BANK_A);
    uint32 crc_B = Calculate_CRC32(BANK_B);
    
    if (crc_A == crc_B) {
        // Already identical, just update flag
        status.bits.banksIdentical = 1;
        Write_DFlash(status);
    } else {
        // Different! Schedule synchronization
        // Will run in background after boot
        status.bits.syncInProgress = 1;
        Write_DFlash(status);
    }
}
```

### 3. 펌웨어 저장 (PFlash) - Dual Bank Layout
```
Bootloader:  0xA0000000 ~ 0xA0000FFF (    4 KB) - Single, Immutable
Bank A:      0xA0001000 ~ 0xA02FFFFF (3,068 KB) - Primary Application
Bank B:      0xA0300000 ~ 0xA05FFFFF (3,072 KB) - Backup/OTA Target
```

### 4. OTA Update Flow
```
Phase 1: Receive Package
  VMG → ZGW (DoIP/UDS) → External Flash (S25FL512S)

Phase 2: Write to Inactive Bank
  External Flash → Internal PFlash (Inactive Bank)
  - Sector-by-sector erase & write
  - CRC32 calculation
  - Watchdog refresh

Phase 3: Update DFlash & Reset
  - Write bootTarget = Inactive Bank
  - Write Bank Status = OK
  - Trigger system reset

Phase 4: Bootloader Boot Selection
  - Read DFlash bootTarget
  - Verify CRC32
  - Jump to selected bank
  - Auto-rollback on failure

Phase 5 (Optional): SW Synchronization
  - After 60 seconds of stable operation
  - Copy Active Bank → Inactive Bank
  - Verify with CRC32
  - Set banksIdentical = 1
```

---

## 메모리 접근 팁

### 읽기
- **Cached 주소 사용**: 빠른 읽기 (코드 실행)
- **Physical 주소 사용**: Flash 프로그래밍 시

### 쓰기
- **Flash**: iLLD API 사용 필수
- **RAM**: 직접 포인터 접근 가능
- **Peripheral**: 레지스터 직접 쓰기

### 보호
- **UCB**: Password로 보호 가능
- **Flash**: UCB_PFLASH로 Write Protection 설정
- **Debug**: UCB_DBG로 JTAG 접근 제어

---

## OTA 관련 데이터 구조

OTA 시스템의 VCI 및 Metadata 구조는 별도 문서로 분리되었습니다:

- **[VCI_information.md](VCI_information.md)**:
  - Level 1: Vehicle-Level VCI
  - Level 2: ECU-Level VCI
  - VCI Collection Flow
  - UDS 접근 방법

- **[Metadata_information.md](Metadata_information.md)**:
  - Level 3-1: Vehicle Package Metadata
  - Level 3-2: Zone Package Metadata
  - Level 3-3: ECU Package Metadata
  - Level 4: Readiness Metadata
  - Package Generation Flow
  - OTA Deployment Sequence

---

## 참고 문서

- **[VCI_information.md](VCI_information.md)**: Vehicle 및 ECU VCI 구조
- **[Metadata_information.md](Metadata_information.md)**: OTA 패키지 메타데이터 계층 구조
- AURIX TC37x User Manual
- TC37x Memory Map (MEMMAP v0.1.21)
- iLLD API Documentation
- FlashBankManager.h (OTA-Zonal_Gateway)

---

## OTA Architecture Diagram

```
┌────────────────────────────────────────────────────────────────┐
│                  Boot Sequence Flow                            │
└────────────────────────────────────────────────────────────────┘
├─────────────────────────────────────────────────────────────┤
│                                                             │
│  Level 1: Vehicle-Level Metadata                           │
│  ├─ VIN, Model, Year                                       │
│  ├─ Total Mileage, Operating Hours                        │
│  ├─ Master SW Version                                      │
│  └─ Network Architecture (Zonal/Domain)                    │
│      Storage: ZGW DFlash (0xAF001000)                      │
│      Size: ~150 bytes                                      │
│                                                             │
│  Level 2: ECU-Level Metadata (VCI)                         │
│  ├─ ECU ID, Serial Number                                  │
│  ├─ SW Version, HW Version                                 │
│  └─ ECU Type, Location                                     │
│      Storage: Each ECU DFlash                              │
│      Size: 48 bytes per ECU                                │
│                                                             │
│  Level 3: OTA Package Metadata                             │
│  ├─ Package Version, Size, CRC                             │
│  ├─ Target ECU List                                        │
│  ├─ Compatibility Matrix                                   │
│  └─ Installation Instructions                              │
│      Storage: External Flash / Package Header              │
│      Size: Variable (typically 256-1024 bytes)             │
│                                                             │
└─────────────────────────────────────────────────────────────┘
```

---

### Level 1: Vehicle-Level Metadata

차량 전체의 식별 및 구성 정보입니다.

#### Vehicle VCI 데이터 구조

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

/* Vehicle Type Definitions */
#define VEHICLE_TYPE_SEDAN          0x01
#define VEHICLE_TYPE_SUV            0x02
#define VEHICLE_TYPE_TRUCK          0x03
#define VEHICLE_TYPE_EV             0x04
#define VEHICLE_TYPE_PHEV           0x05
#define VEHICLE_TYPE_HEV            0x06

/* Region Code Definitions */
#define REGION_CODE_NORTH_AMERICA   0x01  /* US, Canada, Mexico */
#define REGION_CODE_EUROPE          0x02  /* EU, UK, etc */
#define REGION_CODE_ASIA            0x03  /* Korea, Japan, etc */
#define REGION_CODE_CHINA           0x04  /* China specific */
#define REGION_CODE_MIDDLE_EAST     0x05  /* Middle East */

/* Network Architecture Definitions */
#define NETWORK_ARCH_CENTRALIZED    0x01  /* Single central gateway */
#define NETWORK_ARCH_ZONAL          0x02  /* Multiple zonal gateways */
#define NETWORK_ARCH_DOMAIN         0x03  /* Domain-based architecture */
```

#### Vehicle VCI 저장 및 접근

```
Storage Location: 
  - ZGW DFlash: 0xAF001000 (4KB offset from base)
  - Size: 150 bytes
  - Persistence: Survives power cycles and resets
  
UDS Access:
  - Read: 0x22 (ReadDataByIdentifier)
    * DID 0xF190: VIN (17 bytes)
    * DID 0xF191: Vehicle Model (32 bytes)
    * DID 0xF192: Total Mileage (4 bytes)
    * DID 0xF193: Master SW Version (16 bytes)
    * DID 0xF19F: Full Vehicle VCI (150 bytes)
  
  - Write: 0x2E (WriteDataByIdentifier)
    * Requires security access (0x27)
    * Only allowed during manufacturing or service mode
```

---

### Level 2: ECU-Level Metadata (VCI)

개별 ECU의 식별 정보 및 버전 정보입니다.

#### ECU VCI 데이터 구조

```c
/**
 * \brief ECU-Level Configuration Information
 * 
 * Storage: Each ECU's DFlash, 48 bytes
 * Collection: UDP Broadcast/Unicast (13400)
 */
typedef struct
{
    char ecu_id[16];        /* ECU ID (e.g., "ECU_091") */
    char sw_version[8];     /* Software version (e.g., "1.2.3") */
    char hw_version[8];     /* Hardware version (e.g., "0.0.0") */
    char serial_num[16];    /* Serial number (e.g., "091000001") */
} DoIP_VCI_Info;

/* Total size: 48 bytes per ECU */

/* ECU ID Format: "ECU_XYZ"
 *   X: ECU Category
 *     0 = Gateway (ZGW, CGW)
 *     1-7 = Zone ECUs (by zone number)
 *     8 = Body ECUs
 *     9 = ADAS ECUs
 *   
 *   Y: Zone Number (0-9)
 *     0 = Central/Not applicable
 *     1-7 = Zone 1-7
 *     8-9 = Special zones
 *   
 *   Z: ECU Number within category (0-9)
 *     Incremental numbering
 *   
 * Examples:
 *   ECU_091 = Gateway, Zone 9 (central), ECU #1 (ZGW)
 *   ECU_011 = Zone ECU, Zone 1, ECU #1
 *   ECU_812 = Body ECU, Zone 1, ECU #2 (Door control)
 *   ECU_901 = ADAS ECU, Zone 0 (central), ECU #1 (Camera)
 */
```

#### ECU VCI 패킷 구조

**Request (ZGW → Zone ECUs)**:
```
UDP Broadcast to 192.168.1.255:13400
Payload: [0x52, 0x51, 0x53, 0x54]  // "RQST" magic number
Size: 4 bytes
```

**Response (Zone ECU → ZGW)**:
```
UDP Unicast to ZGW:13400
Payload:
  Offset 0-15:  ECU ID (16 bytes, null-terminated string)
  Offset 16-23: SW Version (8 bytes, null-terminated string)
  Offset 24-31: HW Version (8 bytes, null-terminated string)
  Offset 32-47: Serial Number (16 bytes, null-terminated string)
Total Size: 48 bytes
```

---

### Level 3: OTA Package Metadata (Hierarchical)

OTA 패키지는 **3단계 계층 구조**로 구성됩니다: Vehicle → Zone → ECU

#### 계층 구조 개요

```
┌─────────────────────────────────────────────────────────────┐
│  Level 3-1: Vehicle Package (Master Package)                │
│  ├─ Vehicle Metadata (~512 bytes)                           │
│  ├─ Zone Package 1                                          │
│  ├─ Zone Package 2                                          │
│  └─ Zone Package N                                          │
└─────────────────────────────────────────────────────────────┘
              ↓
┌─────────────────────────────────────────────────────────────┐
│  Level 3-2: Zone Package (per Zone)                         │
│  ├─ Zone Metadata (~384 bytes)                              │
│  ├─ ECU Package 1                                           │
│  ├─ ECU Package 2                                           │
│  └─ ECU Package N                                           │
└─────────────────────────────────────────────────────────────┘
              ↓
┌─────────────────────────────────────────────────────────────┐
│  Level 3-3: ECU Package (per ECU)                           │
│  ├─ ECU Metadata (~256 bytes)                               │
│  ├─ Main SW Binary                                          │
│  └─ Add-on Module Binaries (optional)                       │
└─────────────────────────────────────────────────────────────┘
```

---

#### Level 3-1: Vehicle Package Metadata

VMG 서버에서 생성하는 최상위 패키지입니다.

```c
/**
 * \brief Vehicle Package Metadata
 * 
 * Master package containing zone-specific sub-packages
 * Generated by: VMG Server (Vehicle Integration Team)
 * Storage: VMG Server, transmitted to ZGW
 */
typedef struct
{
    /* Package Identification */
    uint32 magic_number;             /* Magic: 0x5650434B ("VPCK") */
    uint16 metadata_version;         /* Metadata structure version */
    uint16 metadata_size;            /* Size of this structure */
    
    /* Package Information */
    char package_id[32];             /* Unique Package ID (e.g., "VPKG_2024_001") */
    char vehicle_sw_version[16];     /* Master SW Version (e.g., "2.5.0") */
    uint32 package_total_size;       /* Total package size (bytes) */
    uint32 package_crc32;            /* CRC32 of entire package */
    uint32 creation_timestamp;       /* Package creation time (Unix timestamp) */
    
    /* Target Vehicle Identification (matches Vehicle-Level VCI) */
    char target_vin[18];             /* Specific VIN (must match Vehicle VCI) */
    char target_vehicle_model[32];   /* Target Model (must match Vehicle VCI) */
    uint16 target_model_year;        /* Target Model Year (0=any) */
    
    /* Compatibility Criteria */
    uint16 target_model_years[8];    /* Applicable model years (0=unused) */
    uint8 target_vehicle_types[8];   /* Applicable vehicle types (0=unused) */
    uint8 target_regions[8];         /* Applicable regions (0=unused) */
    char min_vehicle_sw_version[16]; /* Minimum current vehicle SW version */
    uint8 required_architecture;     /* Required: NETWORK_ARCH_ZONAL (0x02) */
    uint8 required_num_zones;        /* Required number of zones (must match VCI) */
    
    /* Zone Package Information */
    uint8 number_of_zones;           /* Number of zones to update (1-8) */
    struct {
        uint8 zone_id;               /* Zone ID (1-7, 0=Central) */
        char zone_package_id[32];    /* Sub-package ID (e.g., "ZPKG_Z1_001") */
        uint32 zone_package_offset;  /* Offset in vehicle package (bytes) */
        uint32 zone_package_size;    /* Size of zone package (bytes) */
        uint32 zone_package_crc32;   /* CRC32 of zone package */
        uint8 update_required;       /* 0=Skip, 1=Update this zone */
        uint8 update_priority;       /* 0=Low, 1=Med, 2=High, 3=Critical */
        uint8 reserved[2];
    } zone_packages[8];              /* Max 8 zones */
    
    /* Installation Instructions */
    uint8 install_mode;              /* 1=Sequential, 2=Parallel, 3=Staged */
    uint16 max_install_time_sec;     /* Maximum installation time (seconds) */
    uint8 reboot_required;           /* 1=Reboot required after install */
    uint8 rollback_supported;        /* 1=Rollback supported */
    
    /* Security */
    uint8 signature_algorithm;       /* 1=RSA2048, 2=ECDSA256, etc */
    uint16 signature_size;           /* Size of signature (bytes) */
    uint8 signature[256];            /* Digital signature of package */
    
    /* Campaign Information */
    char campaign_id[32];            /* OTA Campaign ID */
    char campaign_name[64];          /* Campaign Name */
    uint8 campaign_priority;         /* 1=Low, 2=Medium, 3=High, 4=Critical */
    uint32 campaign_deadline;        /* Installation deadline (Unix timestamp) */
    
} Vehicle_Package_Metadata;

/* Total size: ~690 bytes (with Vehicle VCI matching fields) */

/* Vehicle Package Magic Number */
#define VEHICLE_PACKAGE_MAGIC       0x5650434B  /* "VPCK" */
```

---

#### Level 3-2: Zone Package Metadata

각 Zone에 배포되는 패키지입니다.

```c
/**
 * \brief Zone Package Metadata
 * 
 * Zone-specific package for a zonal gateway
 * Generated by: VMG Server (Zone Integration Team)
 * Storage: Embedded in Vehicle Package, extracted by ZGW
 */
typedef struct
{
    /* Package Identification */
    uint32 magic_number;             /* Magic: 0x5A50434B ("ZPCK") */
    uint16 metadata_version;         /* Metadata structure version */
    uint16 metadata_size;            /* Size of this structure */
    
    /* Zone Information */
    uint8 zone_id;                   /* Zone ID (1-7, 0=Central) */
    char zone_package_id[32];        /* Package ID (e.g., "ZPKG_Z1_001") */
    char zone_sw_version[16];        /* Zone SW Version */
    uint32 zone_package_size;        /* Total zone package size */
    uint32 zone_package_crc32;       /* CRC32 of zone package */
    
    /* Target Zonal Gateway */
    char target_zgw_id[16];          /* Target ZGW (e.g., "ZGW_01") */
    char target_zgw_sw_version[16];  /* Required ZGW SW version */
    
    /* ECU Package Information */
    uint8 number_of_ecus;            /* Number of ECUs in this zone (1-16) */
    struct {
        char target_ecu_id[16];      /* Target ECU ID (e.g., "ECU_011") */
        char ecu_package_id[32];     /* ECU Package ID */
        char current_sw_version[8];  /* Required current SW version */
        char new_sw_version[8];      /* New SW version after update */
        uint32 ecu_package_offset;   /* Offset in zone package (bytes) */
        uint32 ecu_package_size;     /* Size of ECU package (bytes) */
        uint32 ecu_package_crc32;    /* CRC32 of ECU package */
        uint8 update_sequence;       /* Update order (0=first, 255=last) */
        uint8 critical;              /* 1=Critical (must succeed) */
        uint8 has_addons;            /* 1=Contains add-on modules */
        uint8 reserved;
    } ecu_packages[16];              /* Max 16 ECUs per zone */
    
    /* Zone-Specific Settings */
    uint8 zone_update_mode;          /* 1=Sequential, 2=Parallel */
    uint16 zone_max_time_sec;        /* Maximum time for this zone */
    uint8 zone_rollback_enabled;     /* 1=Zone-level rollback enabled */
    uint8 reserved[3];
    
} Zone_Package_Metadata;

/* Total size: ~800 bytes */

/* Zone Package Magic Number */
#define ZONE_PACKAGE_MAGIC          0x5A50434B  /* "ZPCK" */
```

---

#### Level 3-3: ECU Package Metadata

개별 ECU의 소프트웨어 패키지입니다 (Add-on 모듈 지원).

```c
/**
 * \brief ECU Package Metadata
 * 
 * Individual ECU software package (may contain add-on modules)
 * Generated by: VMG Server (ECU Development Team)
 * Storage: Embedded in Zone Package, extracted by Zone ECU
 */
typedef struct
{
    /* Package Identification */
    uint32 magic_number;             /* Magic: 0x4550434B ("EPCK") */
    uint16 metadata_version;         /* Metadata structure version */
    uint16 metadata_size;            /* Size of this structure */
    
    /* ECU Information */
    char ecu_id[16];                 /* ECU ID (e.g., "ECU_011") */
    char ecu_package_id[32];         /* Package ID */
    char sw_version[16];             /* SW Version */
    uint32 ecu_package_size;         /* Total ECU package size */
    uint32 ecu_package_crc32;        /* CRC32 of ECU package */
    
    /* Main Software */
    char main_sw_version[16];        /* Main SW version */
    uint32 main_sw_offset;           /* Offset in ECU package (bytes) */
    uint32 main_sw_size;             /* Size of main SW (bytes) */
    uint32 main_sw_crc32;            /* CRC32 of main SW */
    uint32 flash_target_address;     /* Target flash address (e.g., 0xA0001000) */
    uint32 flash_size_required;      /* Required flash space (bytes) */
    
    /* Add-on Modules (Optional) */
    uint8 number_of_addons;          /* Number of add-on modules (0-4) */
    struct {
        char addon_id[16];           /* Add-on ID (e.g., "CAM_MODULE") */
        char addon_version[8];       /* Add-on version */
        uint32 addon_offset;         /* Offset in ECU package (bytes) */
        uint32 addon_size;           /* Size of add-on (bytes) */
        uint32 addon_crc32;          /* CRC32 of add-on */
        uint32 addon_flash_address;  /* Target flash address */
        uint8 addon_critical;        /* 1=Critical, 0=Optional */
        uint8 reserved[3];
    } addon_modules[4];              /* Max 4 add-ons per ECU */
    
    /* Installation */
    uint8 install_order;             /* Installation order within zone */
    uint8 requires_bootloader;       /* 1=Requires bootloader mode */
    uint16 install_time_sec;         /* Expected installation time */
    
    /* Compatibility */
    char hw_version_required[8];     /* Required HW version */
    uint32 min_memory_kb;            /* Minimum free memory required */
    
} ECU_Package_Metadata;

/* Total size: ~512 bytes */

/* ECU Package Magic Number */
#define ECU_PACKAGE_MAGIC           0x4550434B  /* "EPCK" */
```

---

#### Hierarchical Package Layout

```
┌─────────────────────────────────────────────────────────────┐
│              Vehicle Package Layout (Master)                │
├─────────────────────────────────────────────────────────────┤
│ Offset       Size        Content                            │
├─────────────────────────────────────────────────────────────┤
│ 0x0000       640 B       Vehicle Package Metadata           │
│                          - Target Criteria                  │
│                          - Zone Package List                │
│                          - Master Signature                 │
├─────────────────────────────────────────────────────────────┤
│ 0x0280       Variable    Zone 1 Package                     │
│                          ┌──────────────────────────────┐   │
│                          │ Zone 1 Metadata (800 B)      │   │
│                          ├──────────────────────────────┤   │
│                          │ ECU Package 1                │   │
│                          │  ├─ ECU Metadata (512 B)     │   │
│                          │  ├─ Main SW Binary           │   │
│                          │  └─ Add-on Binaries (opt)    │   │
│                          ├──────────────────────────────┤   │
│                          │ ECU Package 2                │   │
│                          │  └─ ...                      │   │
│                          └──────────────────────────────┘   │
├─────────────────────────────────────────────────────────────┤
│ ...          Variable    Zone 2 Package                     │
│                          (Same structure as Zone 1)         │
├─────────────────────────────────────────────────────────────┤
│ ...          Variable    Zone N Package                     │
├─────────────────────────────────────────────────────────────┤
│ End          256 B       Master Signature (RSA/ECDSA)       │
└─────────────────────────────────────────────────────────────┘
```

---

#### Package Generation Flow (Server-side)

```
┌─────────────────────────────────────────────────────────────┐
│            VMG Server Package Generation Pipeline           │
└─────────────────────────────────────────────────────────────┘

Step 1: ECU Package Creation (ECU Development Team)
────────────────────────────────────────────────────────────────
Input:  ECU_011.elf, CAM_MODULE.bin, RADAR_MODULE.bin
Output: ECU_011_Package {
          Metadata: ECU_Package_Metadata
          Binary:   Main SW + Add-on 1 + Add-on 2
          CRC32:    Calculated
        }

Step 2: Zone Package Creation (Zone Integration Team)
────────────────────────────────────────────────────────────────
Input:  ECU_011_Package, ECU_012_Package, ECU_013_Package
Output: Zone1_Package {
          Metadata: Zone_Package_Metadata
          Binary:   ECU_011_Pkg + ECU_012_Pkg + ECU_013_Pkg
          CRC32:    Calculated
        }

Step 3: Vehicle Package Creation (Vehicle Integration Team)
────────────────────────────────────────────────────────────────
Input:  Zone1_Package, Zone2_Package, Zone3_Package
Output: Vehicle_Package {
          Metadata: Vehicle_Package_Metadata
          Binary:   Zone1_Pkg + Zone2_Pkg + Zone3_Pkg
          Signature: RSA2048/ECDSA256
          CRC32:    Calculated
        }

Step 4: Deployment
────────────────────────────────────────────────────────────────
VMG Server → Cloud → Vehicle (OTA or Workshop)
```

---

### Level 4: Readiness Metadata

OTA 업데이트 전 ECU 준비 상태 정보입니다.

#### Readiness 데이터 구조

```c
/**
 * \brief Readiness Information for a single ECU
 * 
 * Storage: Runtime collection (RAM), 27 bytes
 * Collection: UDP Broadcast/Unicast (13400)
 * Purpose: Pre-OTA validation of ECU readiness
 */
typedef struct
{
    char ecu_id[16];                 /* ECU ID (e.g., "ECU_091") */
    
    /* Condition Information */
    boolean vehicle_parked;          /* TRUE if gear is P or N */
    boolean engine_off;              /* TRUE if engine/motor is off */
    uint16 battery_voltage_mv;       /* Battery voltage in mV (e.g., 12800 = 12.8V) */
    uint32 available_memory_kb;      /* Available flash memory in KB */
    boolean all_doors_closed;        /* TRUE if all doors are closed */
    
    /* Compatibility (based on VCI) */
    boolean compatible;              /* TRUE if SW is compatible */
    
    /* Final Decision */
    boolean ready_for_update;        /* TRUE if all conditions are met */
    
} Readiness_Info;

/* Total size: 27 bytes per ECU */

/* Readiness Thresholds */
#define MIN_BATTERY_VOLTAGE_MV      12000   /* 12.0V minimum */
#define MIN_AVAILABLE_MEMORY_KB     100     /* 100KB minimum free space */

/* Readiness Decision Logic */
boolean IsReadyForUpdate(Readiness_Info* info)
{
    return (info->vehicle_parked == TRUE) &&
           (info->engine_off == TRUE) &&
           (info->battery_voltage_mv >= MIN_BATTERY_VOLTAGE_MV) &&
           (info->available_memory_kb >= MIN_AVAILABLE_MEMORY_KB) &&
           (info->all_doors_closed == TRUE) &&
           (info->compatible == TRUE);
}
```

#### Readiness 패킷 구조

**Request (ZGW → Zone ECUs)**:
```
UDP Broadcast to 192.168.1.255:13400
Payload: [0x52, 0x44, 0x59, 0x3F]  // "RDY?" magic number
Size: 4 bytes
```

**Response (Zone ECU → ZGW)**:
```
UDP Unicast to ZGW:13400
Payload:
  Offset 0-15:  ECU ID (16 bytes, null-terminated string)
  Offset 16:    vehicle_parked (1 byte, 0/1)
  Offset 17:    engine_off (1 byte, 0/1)
  Offset 18-19: battery_voltage_mv (2 bytes, uint16, big-endian)
  Offset 20-23: available_memory_kb (4 bytes, uint32, big-endian)
  Offset 24:    all_doors_closed (1 byte, 0/1)
  Offset 25:    compatible (1 byte, 0/1)
  Offset 26:    ready_for_update (1 byte, 0/1)
Total Size: 27 bytes
```

---

### Metadata Summary Table

| Level | Type | Size | Storage | Collection | Purpose |
|-------|------|------|---------|------------|---------|
| **1** | Vehicle VCI | 150 bytes | ZGW DFlash (0xAF001000) | Manufacturing/Service | Vehicle identification & history |
| **2** | ECU VCI | 48 bytes | Each ECU DFlash | UDP Broadcast | ECU identification & version |
| **3-1** | Vehicle Package | 640 bytes | VMG Server / Transmitted | Server-generated | Master OTA package |
| **3-2** | Zone Package | 800 bytes | Embedded in Vehicle Pkg | Server-generated | Zone-specific package |
| **3-3** | ECU Package | 512 bytes | Embedded in Zone Pkg | Server-generated | ECU SW + Add-ons |
| **4** | Readiness | 27 bytes | Runtime (RAM) | UDP Broadcast | Pre-OTA validation |

---

## Data Collection & OTA Deployment

### OTA Package Deployment Flow (Hierarchical)

```
┌──────────┐       ┌──────────┐       ┌──────────┐       ┌──────────┐
│   VMG    │       │   ZGW    │       │ Zone ECU │       │  Add-on  │
│  Server  │       │          │       │          │       │  Module  │
└────┬─────┘       └────┬─────┘       └────┬─────┘       └────┬─────┘
     │                  │                  │                  │
     │ ① Vehicle Package│                  │                  │
     │ (All Zones)      │                  │                  │
     ├─────────────────>│                  │                  │
     │   TCP Download   │                  │                  │
     │                  │                  │                  │
     │              ② Parse Vehicle Pkg   │                  │
     │                  ├─────────────┐    │                  │
     │                  │ Extract Zone│    │                  │
     │                  │ Packages    │    │                  │
     │                  │<────────────┘    │                  │
     │                  │                  │                  │
     │                  │ ③ Zone Package  │                  │
     │                  │    (Zone 1)      │                  │
     │                  ├─────────────────>│                  │
     │                  │   TCP/CAN-FD     │                  │
     │                  │                  │                  │
     │                  │              ④ Parse Zone Pkg     │
     │                  │                  ├─────────────┐    │
     │                  │                  │ Extract ECU │    │
     │                  │                  │ Packages    │    │
     │                  │                  │<────────────┘    │
     │                  │                  │                  │
     │                  │                  │ ⑤ ECU Package   │
     │                  │                  │    (with add-ons)│
     │                  │                  ├─────────────────>│
     │                  │                  │   Internal Bus   │
     │                  │                  │                  │
     │                  │              ⑥ Flash Main SW       │
     │                  │                  ├─────────┐        │
     │                  │                  │<────────┘        │
     │                  │                  │                  │
     │                  │                  │ ⑦ Flash Add-on  │
     │                  │                  │                  ├────┐
     │                  │                  │                  │<───┘
     │                  │                  │                  │
     │                  │                  │ ⑧ Verify CRC    │
     │                  │                  ├──────────────────┤
     │                  │                  │<─────────────────┤
     │                  │                  │                  │
     │                  │ ⑨ Zone Complete  │                  │
     │                  │<─────────────────┤                  │
     │                  │   (Success/Fail) │                  │
     │                  │                  │                  │
     │ ⑩ Vehicle Update │                  │                  │
     │    Status        │                  │                  │
     │<─────────────────┤                  │                  │
     │   (All Zones)    │                  │                  │
     │                  │                  │                  │

Package Hierarchy:
┌───────────────────────────────────────────────────────┐
│ Vehicle Package (Master)                              │
│ ├─ Vehicle_Package_Metadata (640 B)                  │
│ ├─ Zone 1 Package                                    │
│ │  ├─ Zone_Package_Metadata (800 B)                 │
│ │  ├─ ECU_011 Package                               │
│ │  │  ├─ ECU_Package_Metadata (512 B)              │
│ │  │  ├─ Main SW Binary                            │
│ │  │  └─ Add-on Binaries (CAM, RADAR)              │
│ │  └─ ECU_012 Package                               │
│ └─ Zone 2 Package                                    │
│    └─ ...                                            │
└───────────────────────────────────────────────────────┘
```

---

### VCI Collection Flow

```
1. VMG → ZGW: UDS 0x31 (Routine Control)
   - SID: 0x31 (RoutineControl)
   - Sub: 0x01 (Start)
   - RID: 0xF001 (CollectVCI)

2. ZGW → Zone ECUs: UDP Broadcast
   - Destination: 192.168.1.255:13400
   - Payload: "RQST" (4 bytes)
   - Timeout: 10 seconds

3. Zone ECUs → ZGW: UDP Unicast
   - Source: ECU IP:13400
   - Payload: VCI_Info (48 bytes)

4. ZGW: Consolidate VCI
   - Store in g_vci_database[MAX_ZONE_ECUS]
   - Count: g_zone_ecu_count

5. ZGW → VMG: UDS 0x22 (ReadDataByIdentifier)
   - DID: 0xF190
   - Response: All collected VCI (48 * N bytes)
```

---

### Readiness Check Flow

```
1. VMG → ZGW: UDS 0x31 (Routine Control)
   - SID: 0x31 (RoutineControl)
   - Sub: 0x01 (Start)
   - RID: 0xF002 (CheckReadiness)

2. ZGW → Zone ECUs: UDP Broadcast
   - Destination: 192.168.1.255:13400
   - Payload: "RDY?" (4 bytes)
   - Timeout: 5 seconds

3. Zone ECUs → ZGW: UDP Unicast
   - Source: ECU IP:13400
   - Payload: Readiness_Info (27 bytes)

4. ZGW: Consolidate Readiness
   - Store in g_readiness_database[MAX_READINESS_ECUS]
   - Check each ECU:
     * Battery >= 12.0V (12000 mV)
     * Memory >= 100KB
     * Vehicle parked (P/N)
     * Engine off
     * All doors closed
     * SW compatible

5. ZGW → VMG: UDS 0x22 (ReadDataByIdentifier)
   - DID: 0xF191
   - Response: Consolidated Readiness (27 * N bytes)
```

---

### Data Collection Sequence Diagram

```
┌─────────────────────────────────────────────────────────────┐
│         OTA Preparation: VCI & Readiness Collection         │
└─────────────────────────────────────────────────────────────┘

Phase 1: VCI Collection
────────────────────────────────────────────────────────────────
VMG (Python)          ZGW (AURIX)         Zone ECUs
   │                      │                     │
   │──UDS 0x31 F001──────>│                     │ 
   │  (CollectVCI)        │                     │
   │                      │                     │
   │                      │──UDP Broadcast─────>│
   │                      │  "RQST" (4 bytes)   │
   │                      │  192.168.1.255:13400│
   │                      │                     │
   │                      │<──UDP Unicast───────│
   │                      │  VCI (48 bytes)     │
   │                      │  ECU IP:13400       │
   │                      │                     │
   │<─UDS 0x71 01─────────│                     │
   │  (Success)           │                     │
   │                      │                     │
   │──UDS 0x22 F190──────>│                     │
   │  (ReadVCI)           │                     │
   │                      │                     │
   │<─VCI Data (48*N)─────│                     │
   │                      │                     │

Phase 2: Readiness Check
────────────────────────────────────────────────────────────────
   │                      │                     │
   │──UDS 0x31 F002──────>│                     │
   │  (CheckReadiness)    │                     │
   │                      │                     │
   │                      │──UDP Broadcast─────>│
   │                      │  "RDY?" (4 bytes)   │
   │                      │  192.168.1.255:13400│
   │                      │                     │
   │                      │<──UDP Unicast───────│
   │                      │  Readiness (27 B)   │
   │                      │  ECU IP:13400       │
   │                      │                     │
   │<─UDS 0x71 01─────────│                     │
   │  (Success)           │                     │
   │                      │                     │
   │──UDS 0x22 F191──────>│                     │
   │  (ReadReadiness)     │                     │
   │                      │                     │
   │<─Readiness (27*N)────│                     │
   │                      │                     │
   │                      │                     │
   │  Decision: All Ready?                      │
   │     ├─ YES → Proceed OTA                   │
   │     └─ NO  → Abort, wait for conditions    │
```

### Network Configuration

```
Network: 192.168.1.0/24

Device         IP Address       UDP Port    Role
─────────────────────────────────────────────────────────────
VMG            192.168.1.10     N/A         OTA Server (Python)
ZGW            192.168.1.100    13400       Zonal Gateway (AURIX)
Zone ECU #1    192.168.1.101    13400       Zone ECU
Zone ECU #2    192.168.1.102    13400       Zone ECU (future)
...
Broadcast      192.168.1.255    13400       UDP Broadcast address
```

### Implementation Files

```
Libraries/DataCollection/
├── vci_manager.h           # VCI collection interface
├── vci_manager.c           # VCI collection implementation
├── readiness_manager.h     # Readiness check interface
└── readiness_manager.c     # Readiness check implementation

Libraries/DoIP/
├── doip_types.h            # VCI structure definition
├── uds_handler.c           # UDS 0x31, 0x22 service handlers
└── doip_client.c           # DoIP protocol

Libraries/Network/
└── UdpEchoServer.c         # UDP broadcast/unicast handling
```

---

## 참고 문서
- AURIX TC37x User Manual
- TC37x Memory Map (MEMMAP v0.1.21)
- iLLD API Documentation
- FlashBankManager.h (OTA-Zonal_Gateway)

---

## OTA Architecture Diagram

```
┌────────────────────────────────────────────────────────────────┐
│                  Boot Sequence Flow                            │
└────────────────────────────────────────────────────────────────┘

Boot ROM (Infineon Hardware)
    ↓
Read UCB_BMHD (0xAF400000)
    ├─ STAD = 0xA0000000 (Fixed!)
    └─ CRC Verification
        ↓
    Jump to 0xA0000000
        ↓
┌──────────────────────────────────────┐
│ Bootloader (4KB)                     │
│ 0xA0000000 ~ 0xA0000FFF              │
│                                      │
│ 1. Read DFlash (0xAF000000)          │
│ 2. Get bootTarget (Bank A/B)         │
│ 3. Verify CRC32                      │
│ 4. Jump to Application               │
└──────────────────────────────────────┘
        ↓
   ┌────┴────┐
   ↓         ↓
┌─────┐   ┌─────┐
│Bank A│   │Bank B│
│0xA00 │   │0xA03 │
│01000 │   │00000 │
└─────┘   └─────┘
   ↓         ↓
Active    Inactive
  App      (OTA)
```

---

**문서 종료**
