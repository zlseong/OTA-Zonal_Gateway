# TC375 Zonal Gateway - Development Notes

## 프로젝트 개요
- **목표**: TC375 Lite Kit 기반 Zonal Gateway 구현
- **아키텍처**: VMG ↔ Zonal Gateway ↔ ECUs (DoIP/UDS, JSON over TCP)
- **하드웨어**: AURIX TC375 Lite Kit (6MB Flash, 512KB RAM, Ethernet PHY)

---

## 하드웨어 정보

### TC375 Lite Kit
- **MCU**: AURIX TC375 (TriCore, 300MHz, 3 cores)
- **메모리**: 
  - PFLASH: 6MB (0x80000000 ~ 0x805FFFFF)
  - RAM: 512KB
  - DFLASH: 384KB
- **Ethernet**: GETH (Gigabit Ethernet Hardware) + PHY
- **LED**: 
  - LED1: P00.5 (Active LOW)
  - LED2: P00.6 (Active LOW)
- **Timer**: STM0 (System Timer, 100MHz)

### 네트워크 설정 (권장)
```c
#define TC375_IP_ADDR    "192.168.1.10"
#define TC375_NETMASK    "255.255.255.0"
#define TC375_GATEWAY    "192.168.1.1"
```

---

## 개발 환경

### 툴체인
- **IDE**: AURIX Development Studio (v1.10.24)
- **컴파일러**: TASKING VX-toolset (non-commercial)
- **디버거**: On-board DAP (USB 연결)
- **빌드 시스템**: Eclipse CDT + amk

### TASKING 컴파일러 제약사항 ⚠️
1. **`#warning` 지시자 미지원** → 제거 필요
2. **`#pragma pack` 미지원** → `__packed` 속성도 불완전
3. **`uintptr_t` 미제공** → `typedef unsigned long uintptr_t;` 수동 정의 필요
4. **Structure packing 불가** → lwIP에서 `MEM_ALIGNMENT=1` 설정으로 우회
5. **빌드 캐시 문제** → 수정 후 `src/` 폴더로 파일 재복사 필요

---

## 검증된 구현 사항

### ✅ LED Blink (Step 1.1)
```c
// P00.5 (LED1), P00.6 (LED2) 교대로 500ms 깜빡임
void initLED(void) {
    IfxPort_setPinMode(LED1_PORT, LED1_PIN, IfxPort_Mode_outputPushPullGeneral);
    IfxPort_setPinMode(LED2_PORT, LED2_PIN, IfxPort_Mode_outputPushPullGeneral);
}
```
- **확인됨**: LED 정상 동작, STM0 타이머 정확도 검증 완료

### ✅ Bare-Metal Architecture (NO_SYS=1)
- **결정**: FreeRTOS 제거, lwIP bare-metal 모드 사용
- **이유**: 
  - FreeRTOS의 GCC 전용 portable 코드가 TASKING과 호환 불가
  - TC375 메모리로 충분히 bare-metal 구현 가능
  - Main loop: Ethernet RX → lwIP timers → Application tasks

---

## 알려진 문제 및 해결책

### 1. lwIP + TASKING 컴파일 이슈
**문제**: Structure packing 미지원으로 프로토콜 헤더 정렬 문제
**해결**: 
```c
// lwipopts.h
#define MEM_ALIGNMENT  1  // Byte-aligned memory allocation

// cc.h
#define PACK_STRUCT_BEGIN
#define PACK_STRUCT_END
#define PACK_STRUCT_STRUCT
```

### 2. uintptr_t 타입 미정의
**해결**:
```c
// cc.h
#ifndef __UINTPTR_T_DEFINED
#define __UINTPTR_T_DEFINED
typedef unsigned long uintptr_t;  // 32-bit for TriCore
typedef long intptr_t;
#endif
```

### 3. iLLD GETH API 불확실성
**문제**: 정확한 함수명/구조체 멤버 미확인
**임시 해결**: Stub 구현 후 하드웨어 테스트 시 수정 필요
```c
// tc375_eth.c에 TODO 주석으로 표시
// TODO: Verify against Libraries/iLLD/TC3xx/Tricore/Geth/Std/IfxGeth.h
```

---

## 다음 구현 단계 (우선순위)

### Phase 1: 네트워크 기본 동작 (2일)
1. **Ethernet + lwIP 통합 완료**
   - GETH 하드웨어 초기화
   - lwIP bare-metal 설정 (NO_SYS=1)
   - ARP, ICMP (ping) 테스트
2. **TCP Echo Server 구현**
   - Raw API 사용 (NO_SYS=1 모드)
   - 간단한 JSON 메시지 송수신 테스트

### Phase 2: DoIP/UDS 기본 구현 (1일)
1. **DoIP 서버** (TCP 13400)
   - Vehicle Identification
   - Routing Activation
   - Diagnostic Message (UDS 0x22 ReadDataByIdentifier)
2. **UDS 기본 서비스**
   - 0x10: Diagnostic Session Control
   - 0x22: Read Data By Identifier (VCI 읽기)

### Phase 3: OTA 준비 (1일)
1. **Dual Bank 메모리 구조**
   - Linker script 수정
   - Bank A/B 전환 로직
2. **Flash 쓰기 테스트**
   - iLLD Flash API 확인
   - Bank B에 데이터 쓰기 테스트

---

## 메모리 맵 (Dual Bank 구조)

```
PFLASH (6MB, 0x80000000 ~ 0x805FFFFF):
├─ 0x80000000 ~ 0x8001FFFF (128KB): SSW (Startup Software)
├─ 0x80020000 ~ 0x8003FFFF (128KB): Bootloader A/B (Dual)
├─ 0x80040000 ~ 0x8004BFFF ( 48KB): Config/Metadata
├─ 0x8004C000 ~ 0x8021BFFF (~1.8MB): Application Bank A ← 현재 실행
└─ 0x8021C000 ~ 0x803EBFFF (~1.8MB): Application Bank B ← OTA 업데이트

RAM (512KB):
├─ CSA (Context Save Area): 64KB
├─ Stacks (per core): 4KB + 4KB + 2KB = 10KB
├─ Heap (lwIP + App): 60KB
└─ Application Data: ~378KB
```

---

## Git Workflow

### 초기화 (현재 작업)
```bash
git reset --hard origin/main  # 메인 브랜치로 복귀
git clean -fdx                # 모든 untracked 파일 제거
```

### 점진적 개발
```bash
# 기능별 커밋
git add Cpu0_Main.c
git commit -m "Add LED blink test"

git add lwip/
git commit -m "Add lwIP bare-metal integration"
```

---

## 참고 문서
- **System Architecture**: [VMG_and_MCUs](https://github.com/zlseong/VMG_and_MCUs.git) `/docs`
- **TC375 iLLD**: `Libraries/iLLD/TC3xx/Tricore/`
- **lwIP Documentation**: https://www.nongnu.org/lwip/2_1_x/
- **GitHub Repository**: [AURIX-TC375-Lite-Kit](https://github.com/zlseong/AURIX-TC375-Lite-Kit.git)

---

## 개발 로그

### 2025-11-01
- ✅ LED blink 검증 완료 (P00.5, P00.6)
- ✅ FreeRTOS → Bare-metal 전환 결정
- ❌ lwIP + TASKING 컴파일러 호환성 이슈 다수 발견
- **결정**: 프로젝트 초기화 후 단계별 재구현 (✅ 완료)

---

## 중요 사항
1. **TASKING 컴파일러 특성을 항상 고려** (structure packing, uintptr_t 등)
2. **iLLD API는 하드웨어 테스트 전까지 Stub 구현**
3. **Bare-metal 아키텍처로 단순화** (RTOS 없음)
4. **단계별 검증**: LED → Ethernet → DoIP → OTA
5. **Git 커밋은 작은 단위**로 (기능별)

---

*이 문서는 개발 진행 상황에 따라 지속적으로 업데이트됩니다.*

