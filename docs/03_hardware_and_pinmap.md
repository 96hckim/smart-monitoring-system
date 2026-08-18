# 하드웨어 사양 및 핀맵 정의서 (Hardware Specification & Pinmap v1.0)

본 문서는 **스마트 가스 모니터링 시스템**의 엣지 노드(STM32F411RE)를 구성하는 센서 입력부, 액추에이터 구동부, 마이크로컨트롤러 핀 할당, 회로 인터페이스 규격, 그리고 전원 무결성(Power Integrity) 설계를 정의합니다.

---

## 📋 목차

1. 하드웨어 시스템 개요 및 구성도
2. STM32F411RE 핀 할당 명세 (Pinout)
3. 가스 센서 계측 회로 및 ADC 인터페이스
4. 액추에이터 구동 회로 및 전기적 제어 사양
5. 전원 분리 및 전압 강하(Voltage Sag) 방어 회로
6. 하드웨어 결선 다이어그램 및 배선 점검표

---

## 1. 하드웨어 시스템 개요 및 구성도

엣지 노드는 96MHz로 동작하는 **STM32F411RE Nucleo-64**를 중심으로, 1채널 12비트 아날로그 가스 센서 입력과 3중 액추에이터(물리 밸브 서보 모터, DC 환기 팬, 고휘도 경보 LED) 출력 회로로 구성됩니다.

```text
                                  +-----------------------+
                                  | STM32F411RE Nucleo-64 |
                                  |   (96MHz Bare-metal)  |
                                  +-----------+-----------+
                                              |
      +---------------------+-----------------+---------------------+---------------------+
      | (ADC1_IN6 / PA6)    | (TIM2_CH1 / PA0)| (GPIO Output / PA1) | (GPIO Output / PA4) | (USART2 / PA2, PA3)
      ▼                     ▼                 ▼                     ▼                     ▼
+-------------+      +--------------+  +---------------+      +-------------+      +---------------+
| Gas Sensor  |      |  SG90 Servo  |  | DC Fan Motor  |      |  Alarm LED  |      | Qt PC Gateway |
| (Analog In) |      | (Valve Shut) |  | (Ventilation) |      | (Visual Warn|      | (USB VCP Com) |
+-------------+      +--------------+  +---------------+      +-------------+      +---------------+
```

---

## 2. STM32F411RE 핀 할당 명세 (Pinout)

모든 GPIO 핀은 레지스터 직접 제어로 설정되며, MCU 내부 풀업/풀다운 레지스터 및 Alternate Function 멀티플렉서를 사용합니다.

```text
                  [ STM32F411RE Nucleo-64 (CN7/CN10) ]
                        +--------------------+
        (TIM2_CH1)  PA0 |  1 (CN7)  28 (CN10)| PA6  (ADC1_IN6 - Gas Sensor)
        (DC Fan)    PA1 |  2 (CN7)  27 (CN10)| PA5  (Board User LED)
        (USART2_TX) PA2 |  3 (CN7)  26 (CN10)| PA4  (Alarm Red LED)
        (USART2_RX) PA3 |  4 (CN7)  25 (CN10)| +5V  (External 5V Power)
                   +3V3 |  5 (CN7)  24 (CN10)| GND  (Common Power GND)
                    GND |  6 (CN7)  23 (CN10)| GND  (Analog GND)
                        +--------------------+
```

### 2.1 핀 기능 및 전기적 특성 표

| 핀 번호 | 포트 모드 (MODER) | Alternate Function | 신호 레벨 | 드라이브 모드 | 연결 장치 및 세부 기능 |
| --- | --- | --- | --- | --- | --- |
| **PA0** | Alternate Function (`10b`) | **AF1** (TIM2) | 3.3V Logic (5V Servo) | Push-Pull, High-Speed | **SG90 서보 모터 PWM 제어 신호선** (50Hz 펄스) |
| **PA1** | Output (`01b`) | — | 3.3V Logic | Push-Pull, Low-Speed | **DC 환기 팬 모터 제어 신호** (High: Fan ON) |
| **PA2** | Alternate Function (`10b`) | **AF7** (USART2) | 3.3V TTL | Push-Pull, High-Speed | **USART2 TX** (ST-Link USB VCP $\rightarrow$ Qt 서버) |
| **PA3** | Alternate Function (`10b`) | **AF7** (USART2) | 3.3V TTL | Floating Input | **USART2 RX** (Qt 서버 $\rightarrow$ 밸브 제어 수신) |
| **PA4** | Output (`01b`) | — | 3.3V Logic | Push-Pull, Low-Speed | **위험 경보 Red LED 제어** (High: 점등) |
| **PA6** | Analog Mode (`11b`) | — | $0.0\text{V} \sim 3.3\text{V}$ | High-Z Analog | **가스 센서 아날로그 입력** (ADC1 Channel 6) |

---

## 3. 가스 센서 계측 회로 및 ADC 인터페이스

가스 센서(또는 정밀 분압 가변저항 모듈)는 가스 농도에 따른 전압 변화를 출력하며, MCU 내부의 12비트 SAR(Successive Approximation Register) ADC로 직접 인가됩니다.

```text
        +3.3V (MCU VDD / Analog Reference)
          │
         ┌┴┐
         │ │  가스 검출 센서 (Rs)
         └┬┘
          ├────────── PA6 (ADC1_IN6 아날로그 입력)
         ┌┴┐
         │ │  로드 저항 (RL, 10kΩ)
         └┬┘
          │
         GND (Analog Ground)
```

### 3.1 ADC 레지스터 설정 파라미터

* **해상도**: 12-bit (0 ~ 4095)
* **클럭 소스**: APB2 Clock (96MHz) $\div$ Prescaler 6 = **16MHz ADC Clock**
* **샘플링 주기**: 480 ADC Cycles (`SMPR2[20:18] = 111b`)
* **변환 시간 계산**:

$$T_{conv} = \text{Sampling Time} + 12\text{ Cycles} = 480 + 12 = 492\text{ Cycles}$$


$$T_{total} = \frac{492}{16\text{ MHz}} = 30.75\mu\text{s}$$


* **전압 변환 수식**:

$$V_{sensor} = \frac{\text{ADC\_Value}}{4095} \times 3.3\text{ V}$$



---

## 4. 액추에이터 구동 회로 및 전기적 제어 사양

### 4.1 SG90 서보 모터 (물리 밸브)

TIM2 채널 1의 16비트 타이머 하드웨어 PWM을 사용하여 서보 모터의 각도를 제어합니다.

```text
    TIM2 PWM Pulse Output (PA0, 50Hz / 20ms Period)
    
    [0도 개방 펄스: 0.5ms]
    +---+
    |   |___________________________________________ (19.5ms Low)
    <---> 500us (CCR1 = 500)
    
    [90도 차단 펄스: 1.5ms]
    +---------+
    |         |_____________________________________ (18.5ms Low)
    <-------> 1500us (CCR1 = 1500)
```

* **타이머 클럭**: 96MHz (APB1 Prescaler x2 적용)
* **프리스케일러 (PSC)**: $96 - 1 = 95$ ($1\text{ tick} = 1\mu\text{s}$)
* **자동 재로드 레지스터 (ARR)**: $20000 - 1 = 19999$ ($20\text{ms}$ 주기)

| 밸브 상태 | 목표 각도 | 제어 펄스 폭 ($T_{on}$) | TIM2->CCR1 설정값 | 물리적 상태 및 안전 의미 |
| --- | --- | --- | --- | --- |
| **정상 (개방)** | **$0^\circ$** | $500\mu\text{s}$ ($0.5\text{ms}$) | `500` | 가스 배관 개방 (정상 공급 상태) |
| **위험 (차단)** | **$90^\circ$** | $1500\mu\text{s}$ ($1.5\text{ms}$) | `1500` | 가스 배관 차단 (가스 누출 방지) |

---

### 4.2 환기용 DC 팬 모터 구동 회로

DC 모터의 역기전력(Back-EMF)과 돌입 전류를 안전하게 제어하기 위해 NPN 트랜지스터(또는 모터 드라이버 채널)와 플라이백 다이오드를 적용합니다.

```text
           +5V (Main Power Rail)
             │
             ├───[ DC Fan Motor ]───┐
             │                      │
            [D1] 1N4007             ├───── Collector (2N2222 / SS8050)
             │ (Flyback Diode)      │
             └──────────────────────┘
                                    │
    PA1 (3.3V) ───[ 1kΩ Resistor ]──┤ Base
                                    │
                                 Emitter
                                    │
                                   GND
```

* **GPIO High 출력 (`PA1 = 1`)**: Base에 약 $2.6\text{mA}$ 전류가 인가되어 트랜지스터가 포화(Saturation) 상태로 진입, DC 팬 가동.
* **플라이백 다이오드(D1)**: 모터가 꺼질 때 코일에서 발생하는 역기전력 고전압 스파이크($-L \frac{di}{dt}$)를 5V 라인으로 환류시켜 트랜지스터와 MCU 보호.

---

### 4.3 위험 경보 LED 회로

`PA4` 핀으로 5mm 고휘도 Red LED를 Push-Pull 구동합니다.

```text
    PA4 (3.3V Logic) ───[ 220Ω (1/4W) ]───(▶| Red LED )─── GND
```

* **순방향 전압 ($V_F$)**: 약 2.0V
* **제한 저항 ($R$)**: $220\Omega$
* **도통 전류 ($I_F$)**:

$$I_F = \frac{3.3\text{ V} - 2.0\text{ V}}{220\Omega} \approx 5.9\text{ mA} \quad (\text{MCU 핀당 최대 허용 전류 25mA 이하 준수})$$



---

## 5. 전원 분리 및 전압 강하(Voltage Sag) 방어 회로

모터 구동체(SG90 서보, DC 팬)는 동작 순간 **0.5A ~ 1.0A 이상의 서지 전류**를 소모합니다. 동일 전원선 사용 시 발생하는 전압 강하(Voltage Sag)로 인해 ADC 기준 전압($V_{REF}$)이 흔들려 센서 측정값이 왜곡되는 현상을 방지하기 위해 다음과 같은 전원 분리 설계를 적용합니다.

```text
  +5V External Power ───────┬───────────────────────────────┬───────────────────────────┐
                            │                               │                           │
                     [ 470uF / 16V ]                 [ 100uF / 16V ]             [ 0.1uF Ceramic ]
                     (Bulk Electrolytic)             (Bulk Electrolytic)         (Decoupling)
                            │                               │                           │
                            ▼                               ▼                           ▼
                     [ SG90 Servo ]                  [ DC Fan Motor ]            [ Nucleo Board 5V ]
                            │                               │                           │
                            ▼                               ▼                           ▼
  Common GND ───────────────┴───────────────────────────────┴───────────────────────────┴──────── GND
```

### 5.1 전원 안정화 3단계 규칙

1. **전해 캐패시터(Bulk Capacitor)**:
* 서보 모터 전원단: **$470\mu\text{F} / 16\text{V}$**
* DC 팬 모터 전원단: **$100\mu\text{F} / 16\text{V}$**
* 급격한 모터 스위칭 시 발생하는 순간 전압 강하를 축전된 전하로 보충.


2. **세라믹 디커플링 캐패시터(Decoupling Capacitor)**:
* 센서 및 MCU 전원 입력단에 **$0.1\mu\text{F} (104)$** 캐패시터를 병렬 배치하여 모터 브러시에서 유입되는 고주파 스파크 노이즈 차단.


3. **단일점 공통 접지(Star Ground)**:
* 모터 대전류가 흐르는 접지선과 MCU/센서의 아날로그 접지선을 물리적으로 분리하여 전원 공급원(GND 핀) 근처에서 한 지점으로 결합(Ground Bounce 억제).



---

## 6. 하드웨어 결선 다이어그램 및 배선 점검표

### 6.1 배선 결선 명세표

| 구분 | 장치명 | 장치 핀 / 와이어 색상 | Nucleo 연결 핀 | 비고 |
| --- | --- | --- | --- | --- |
| **센서** | 가스 센서 모듈 | VCC (Red) | `+3V3` | 아날로그 기준 전원 공유 |
|  |  | GND (Black) | `GND` | 아날로그 접지 |
|  |  | AOUT (Yellow/Green) | `PA6` | ADC1 Channel 6 입력 |
| **서보** | SG90 서보 모터 | VCC (Red) | `5V` (Ext / Board) | $470\mu\text{F}$ 캐패시터 병렬 연결 |
|  |  | GND (Brown) | `GND` | 공통 전원 접지 |
|  |  | PWM Signal (Orange) | `PA0` | TIM2_CH1 PWM 출력 |
| **DC 팬** | 팬 구동 모듈 | VCC (+) | `5V` | $100\mu\text{F}$ 캐패시터 병렬 연결 |
|  |  | GND (-) | Transistor Collector | 1N4007 다이오드 병렬 |
|  |  | Control In | `PA1` | $1\text{k}\Omega$ 저항 직렬 연결 |
| **LED** | 경보 Red LED | Anode (+) | `PA4` | $220\Omega$ 저항 직렬 연결 |
|  |  | Cathode (-) | `GND` | 공통 접지 |