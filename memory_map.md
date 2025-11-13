# AURIX TC37x Memory Map Reference

## 목차
1. [UCB (User Configuration Block)](#ucb-user-configuration-block)
2. [DFlash (Data Flash)](#dflash-data-flash)
3. [PFlash (Program Flash)](#pflash-program-flash)
4. [RAM (DLMU, SRAM, etc)](#ram-memory)
5. [Peripheral Registers](#peripheral-registers)

---

## UCB (User Configuration Block)

### 주소 범위
- **Base**: `0xAF400000` ~ `0xAF405FFF` (24 KB)

### UCB 구성 요소

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
- **Base**: `0xAFC00000` ~ `0xAFC1FFFF` (128 KB)
- **EEPROM 에뮬레이션**: DF1 (Data Flash 1)
- **HSM Command**: HSM Command Sequence Interpreter

### 특징
- **읽기**: ok
- **쓰기**: ok (Page 단위)
- **용도**: 
  - 설정 데이터 저장
  - Calibration 데이터
  - OTA 상태 정보 (7비트 플래그)
  - NVM (Non-Volatile Memory)

### DFlash 프로그래밍
- **Page 크기**: 32 bytes (일부 영역 512 bytes)
- **API**: `IfxFlash_enterPageMode()`, `IfxFlash_loadPage2X32()`

---

## PFlash (Program Flash)

### TC37x PFlash Bank 구조

| Bank | 주소 범위 (Physical) | 주소 범위 (Cached) | 크기 | 용도 |
|------|---------------------|-------------------|------|------|
| **PFlash0 (Bank A)** | 0xA0000000 ~ 0xA02FFFFF | 0x80000000 ~ 0x802FFFFF | 3 MB | Program Code (Primary) |
| **PFlash1 (Bank B)** | 0xA0300000 ~ 0xA05FFFFF | 0x80300000 ~ 0x805FFFFF | 3 MB | Program Code (Backup/Golden) |
| **Total** | - | - | **6 MB** | - |

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

### 1. Boot 설정 (UCB)
```
0xAF400000: BMHD0_ORIG
  - .stad = 0xA0000000 (Bank A) 또는 0xA0300000 (Bank B)
  - CRC 포함 (필수)
```

### 2. 상태 저장 (DFlash)
```
0xAFC00000: 7비트 플래그
  - Bit 0: 현재 뱅크 (0=A, 1=B)
  - Bit 1: Boot Flag A
  - Bit 2: Boot Flag B
  - Bit 3: Error A
  - Bit 4: Error B
  - Bit 5: Banks Identical
  - Bit 6: Critical Error
```

### 3. 펌웨어 저장 (PFlash)
```
Bank A: 0xA0000000 ~ 0xA02FFFFF (3MB) - Primary
Bank B: 0xA0300000 ~ 0xA05FFFFF (3MB) - Golden/Backup
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

## 참고 문서
- AURIX TC37x User Manual
- TC37x Memory Map (MEMMAP v0.1.21)
- iLLD API Documentation

---

**최종 업데이트**: 2025-01-12
**프로젝트**: OTA-Zonal_Gateway (TC375 Lite Kit)

