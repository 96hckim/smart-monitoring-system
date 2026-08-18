# README.md

```markdown
<div align="center">

# 🛡️ Smart Gas Monitoring & Safety Response System
### 스마트 산업용 가스 모니터링 및 실시간 원격 안전 차단 시스템

[![STM32](https://img.shields.io/badge/STM32F411RE-ARM%20Cortex--M4-002B49?logo=stmicroelectronics)](https://www.st.com/)
[![Bare-metal C](https://img.shields.io/badge/Firmware-Bare--metal%20C99-00599C?logo=c)](stm32-firmware/)
[![Qt](https://img.shields.io/badge/Gateway-Qt%205%20%2F%20C%2B%2B17-41CD52?logo=qt)](qt-server/)
[![Android](https://img.shields.io/badge/Client-Kotlin%20%2F%20Compose-3DDC84?logo=android)](android/)
[![CameraX](https://img.shields.io/badge/Streaming-CameraX%201.4-FFA000?logo=google)](https://developer.android.com/training/camerax)
[![Tailscale](https://img.shields.io/badge/Network-Tailscale%20VPN-4969ED?logo=tailscale)](https://tailscale.com/)

<p align="center">
  <b>96MHz 베어메탈 엣지 계측</b>부터 <b>Qt 데스크톱 게이트웨이</b>, <b>저지연 모바일 영상 스트리밍</b>까지<br>
  산업 현장의 가스 누출을 200ms 주기로 감시하고 즉각적인 물리적 안전 조치를 실행하는 3계층 End-to-End 솔루션
</p>

</div>

---

## 📌 목차
1. [프로젝트 개요](#-프로젝트-개요)
2. [시스템 아키텍처 및 데이터 흐름](#-시스템-아키텍처-및-데이터-흐름)
3. [핵심 엔지니어링 기능](#-핵심-엔지니어링-기능)
4. [하드웨어 사양 및 핀맵](#-하드웨어-사양-및-핀맵)
5. [통신 프로토콜 요약](#-통신-프로토콜-요약)
6. [빠른 시작 가이드 (Quick Start)](#-빠른-시작-가이드-quick-start)
7. [프로젝트 디렉터리 구조](#-프로젝트-디렉터리-구조)
8. [상세 기술 문서 (Docs)](#-상세-기술-문서-docs)

---

## 📖 프로젝트 개요

본 프로젝트는 밀폐 공간 및 산업 현장에서 발생할 수 있는 가스 누출 위험을 실시간으로 감지하고, **위험 상황 시 물리 밸브 차단, 강제 환기, 시각 경보를 자율적/원격으로 동시 수행**하는 스마트 안전 관제 시스템입니다.

![전체 시스템 하드웨어 셋업](images/hardware_setup.png)

* **결정론적 엣지 제어**: 상용 RTOS/HAL 라이브러리를 배제하고 레지스터 직접 제어로 구축된 STM32 베어메탈 펌웨어를 통해 200ms 주기 고신뢰성 가스 계측 및 하드웨어 PWM 액추에이션을 수행합니다.
* **중앙 집중식 게이트웨이**: Qt 기반 데스크톱 관제 서버가 시리얼(UART)과 네트워크(TCP)를 중계하며, 위험 수치 감지 시 단일 트리거 래치 방식으로 밸브를 자동 차단하고 실시간 시계열 그래프를 렌더링합니다.
* **원격 모바일 관제 & CCTV 스트리밍**: 현장 점검자는 안드로이드 앱을 통해 현장 영상을 저지연으로 실시간 송출하고, Tailscale 오버레이 VPN을 통해 외부 LTE/5G 환경에서도 원격으로 현장을 감시 및 수동 제어합니다.

---

## 📐 시스템 아키텍처 및 데이터 흐름

시스템은 **Edge Node(센서/액추에이터) $\leftrightarrow$ Gateway(데스크톱 관제) $\leftrightarrow$ Mobile Client(영상/원격 제어)**의 완벽한 3계층 구조로 분리되어 높은 응집도와 낮은 결합도를 유지합니다.

```text
[ 현장 작업자 / 모바일 클라이언트 ]
  │  Android 8.0+ (Jetpack Compose + CameraX)
  │  - 저지연 JPEG 비디오 스트리밍 (NV21 -> JPEG 압축)
  │  - 실시간 가스 농도 텔레메트리 표시 및 원격 밸브 제어 ('1' / '0')
  │
  │  ▲ [Mode 1: Local Wi-Fi (192.168.137.1:8080)]
  │  ▼ [Mode 2: Tailscale Mesh VPN (100.72.78.11:8080 - NAT Traversal)]
  │
[ 중앙 관제 센터 / 데스크톱 게이트웨이 ]
  │  Qt 5/6 Control Center (C++17 Widgets)
  │  - 4바이트 Big-Endian 프레임 디멀티플렉싱 & CCTV 모니터링
  │  - 50개 샘플 슬라이딩 윈도우 시계열 차트 & 일별 CSV 데이터 로깅
  │  - 안전 임계값(Threshold) 비교 및 자동 차단 단일 트리거 래치
  │
  │  ▲ UART 115200 8N1 (USB VCP)
  │  ▼ Non-blocking ASCII 텔레메트리 ("1265\n") & 1바이트 제어 명령
  │
[ 엣지 센싱 & 안전 액추에이션 노드 ]
  │  STM32F411RE Nucleo-64 (96MHz PLL Bare-metal)
  │  - 12-bit SAR ADC (PA6): 200ms 주기 가변저항 가스 농도 계측
  │  - TIM2_CH1 Hardware PWM (PA0): SG90 서보 물리 밸브 개폐 (0° / 90°)
  │  - Motor Driver (PC0, PC1): DC 환기 팬 모터 정회전/정지 제어
  │  - 차단벽 경보 LED (PB0, PB1) & 현장 수동 차단/복구 키 (PB4, PB5)
```

---

## 🚀 핵심 엔지니어링 기능

### 1. 엣지 노드 (STM32 Bare-metal)

* **96MHz 최대 클럭 트리 및 Flash 가속**: HSI 16MHz를 PLL 체배하여 96MHz 구동, Flash 3 Wait States 및 Prefetch / I-Cache / D-Cache를 활성화하여 메모리 병목 제거.
* **TIM2 50Hz 하드웨어 PWM 서보 제어**: 1µs 분해능 타이머로 SG90 모터를 $0^\circ(500\mu\text{s})$ 개방 및 $90^\circ(1500\mu\text{s})$ 차단 각도로 정밀 제어.
* **상태 가드 소프트웨어 인터록**: 동일한 제어 명령 재유입 시 하드웨어 레지스터 덮어쓰기를 차단하여 모터 떨림 및 서지 전류 유발 방지.
* **전원 무결성(Power Integrity) 설계**: 서보/모터 기동 시 발생하는 전압 강하(Voltage Sag)로 인한 ADC $V_{\text{REF}}$ 왜곡을 방지하기 위해 벌크 캐패시터($470\mu\text{F} / 100\mu\text{F}$) 및 단일점 공통 접지(Star Ground) 적용.

### 2. 게이트웨이 (Qt Control Center)

* **이종 매체 간 비동기 브리지**: UART 시리얼 데이터와 TCP 네트워크 패킷을 UI 블로킹 없이 중계하는 시그널-슬롯 비동기 이벤트 라우팅.
* **단일 트리거 안전 래치**: 가스 농도가 위험 임계값(기본 3000)을 초과하는 순간 1회만 차단 명령(`'1'`)을 전송하여 통신 채널 부하 최소화.
* **4바이트 Big-Endian 패킷 프레이밍**: TCP 스트림 단편화에 대응하여 `[Length: 4B][JPEG Data]` 바이너리와 텍스트 제어 명령(`'1'`, `'0'`)을 단일 포트에서 안전하게 분기/디코딩.

### 3. 모바일 클라이언트 (Android)

* **초저지연 CameraX 영상 파이프라인**: `STRATEGY_KEEP_ONLY_LATEST` 배압 전략과 YUV-to-NV21 하드웨어 메모리 재구성을 통해 버퍼 지연 없는 실시간 영상 스트리밍 구현.
* **Tailscale 오버레이 VPN 지원**: 공인 IP나 복잡한 포트포워딩(DDNS) 없이 스마트폰이 LTE/5G 이동통신망에 연결되어 있어도 현장 PC 게이트웨이(`100.72.78.11`)와 안전하게 P2P 암호화 터널링 통신.

---

## 🔌 하드웨어 사양 및 핀맵

| 핀 번호 | 주변장치 모드 | 신호 레벨 | 드라이브/입력 모드 | 연결 장치 및 기능 설명 |
| --- | --- | --- | --- | --- |
| **PA0** | TIM2_CH1 (AF1) | 3.3V Logic | Push-Pull, High-Speed | **SG90 서보 모터 PWM** (50Hz 펄스, 밸브 0°/90° 제어) |
| **PA2** | USART2_TX (AF7) | 3.3V TTL | Push-Pull, High-Speed | **UART 송신선** (ST-Link USB VCP $\rightarrow$ Qt 서버) |
| **PA3** | USART2_RX (AF7) | 3.3V TTL | Floating Input | **UART 수신선** (Qt 서버 제어 명령 수신) |
| **PA6** | ADC1_IN6 (Analog) | 0.0V ~ 3.3V | High-Z Analog | **가스 센서 아날로그 입력** (12비트 SAR, 가변저항) |
| **PB0** | GPIO Output | 3.3V Logic | Push-Pull, Low-Speed | **차단벽 경보 LED 1** (High: 점등) |
| **PB1** | GPIO Output | 3.3V Logic | Push-Pull, Low-Speed | **차단벽 경보 LED 2** (High: 점등) |
| **PB4** | GPIO Input | 3.3V Logic | Pull-Up Input | **KEY1: 현장 수동 밸브 즉시 차단** (Active-Low) |
| **PB5** | GPIO Input | 3.3V Logic | Pull-Up Input | **KEY2: 현장 수동 밸브 정상 복구** (Active-Low) |
| **PC0** | GPIO Output | 3.3V Logic | Push-Pull, Low-Speed | **모터 드라이버 IN1** (DC 환기 팬 정회전 구동) |
| **PC1** | GPIO Output | 3.3V Logic | Push-Pull, Low-Speed | **모터 드라이버 IN2** (DC 환기 팬 정지/제동) |

---

## 📡 통신 프로토콜 요약

### 1. STM32 $\leftrightarrow$ Qt 서버 (UART 115200 8N1)

* **상향 (센서 계측)**: `1265\n` (200ms 주기 ASCII 정수 전송)
* **하향 (액추에이터 제어)**: `'1'` (위험 차단: 밸브 90°, 팬 ON, LED ON) / `'0'` (정상 복구)

### 2. Qt 서버 $\leftrightarrow$ Android 클라이언트 (TCP/IP Port 8080)

* **영상 프레임 (Android $\rightarrow$ Qt)**: `[4-Byte Big-Endian Payload Size][JPEG Binary]`
* **수동 제어 명령 (Android $\rightarrow$ Qt)**: `'1\n'` (원격 차단) / `'0\n'` (원격 복구)
* **가스 텔레메트리 (Qt $\rightarrow$ Android)**: `GAS:<ADC_Value>:<Threshold>\n` (예: `GAS:1265:3000\n`)

---

## ⚡ 빠른 시작 가이드 (Quick Start)

### 1. STM32 펌웨어 빌드 및 플래싱

```bash
cd stm32-firmware
make clean && make
# 생성된 main.bin을 Nucleo 가상 USB 드라이브로 복사하거나 ST-Link CLI로 플래싱
```

### 2. Qt 관제 서버 빌드 및 실행

```bash
# MSYS2 UCRT64 환경 필수 의존성 설치
pacman -S mingw-w64-ucrt-x86_64-qt5-serialport mingw-w64-ucrt-x86_64-qt5-charts

cd qt-server
qmake qt-server.pro
mingw32-make -j4       # Linux의 경우: make -j4
./release/SmartMonitoringServer.exe
```

### 3. Android 모바일 클라이언트 빌드

```bash
cd android
./gradlew assembleDebug
# app/build/outputs/apk/debug/app-debug.apk 디바이스 설치
```

---

## 📂 프로젝트 디렉터리 구조

```text
.
├── android/                   # Android 클라이언트 (Kotlin, Jetpack Compose, CameraX)
│   ├── app/src/main/java/com/example/smartmonitoring/
│   │   ├── data/              # TCP 소켓 엔진 (TcpSocketClient.kt)
│   │   ├── ui/                # Compose 화면 구성 (MonitoringScreen.kt, ViewModel.kt)
│   │   └── util/              # NV21 to JPEG 이미지 변환 파이프라인 (ImageUtils.kt)
│   └── build.gradle.kts
├── qt-server/                 # Qt 데스크톱 관제 서버 (C++17, Qt Widgets, QtCharts)
│   ├── src/                   # 통신 및 UI 모듈 (MainWindow, SerialManager, TcpStreamServer)
│   ├── qt-server.pro          # qmake 프로젝트 빌드 설정 파일
│   └── config.ini             # 시스템 영속 설정 파일 (동적 생성)
├── stm32-firmware/            # STM32 베어메탈 펌웨어 (C99, CMSIS Register)
│   ├── src/                   # 드라이버 구현부 (adc.c, timer.c, uart.c, led.c, motor.c, key.c)
│   ├── Makefile               # arm-none-eabi-gcc 빌드 스크립트
│   └── STM32F411RETx_FLASH.ld # 링커 스크립트
├── images/                    # 하드웨어 결선도, 밸브 동작 비교 및 UI 스크린샷
└── docs/                      # 시스템 기술 문서 세트 (Architecture, Protocol, Pinmap 등)
```

---

## 📚 상세 기술 문서 (Docs)

시스템의 세부 설계 및 구현 명세는 `docs/` 디렉터리의 기술 문서를 참고하십시오.

| 번호 | 문서명 | 주요 내용 |
| --- | --- | --- |
| **01** | [시스템 아키텍처 정의서](https://www.google.com/search?q=docs/01_system_architecture.md) | 3계층 구조, Tailscale/Hotspot 네트워크 토폴로지, Mermaid 시퀀스 다이어그램 |
| **02** | [통신 프로토콜 명세서](https://www.google.com/search?q=docs/02_protocol_specification.md) | UART ASCII 및 TCP 4바이트 길이 헤더 프레이밍 규격 |
| **03** | [하드웨어 사양 및 핀맵](https://www.google.com/search?q=docs/03_hardware_and_pinmap.md) | MCU 핀맵, 액추에이터 구동 회로, 전원 노이즈 및 Star Ground 설계 |
| **04** | [STM32 펌웨어 드라이버 가이드](https://www.google.com/search?q=docs/04_firmware_driver_guide.md) | 96MHz 클럭 트리, ADC1, TIM2 PWM, USART2, 소프트웨어 인터록 |
| **05** | [Qt 관제 서버 아키텍처](https://www.google.com/search?q=docs/05_qt_control_center.md) | 시그널-슬롯 비동기 중계, 슬라이딩 차트, INI/CSV 데이터 로깅 |
| **06** | [시스템 설치 및 운용 가이드](https://www.google.com/search?q=docs/06_setup_and_operation_guide.md) | 개발 환경 구축, MSYS2 패키지 설치, 4단계 통합 검증 시나리오 |
| **07** | [트러블슈팅 및 기술 FAQ](https://www.google.com/search?q=docs/07_troubleshooting_and_faq.md) | 전압 강하, 통신 타임아웃, 메모리 누수 방어 등 문제 해결 가이드 |

---