#include "device_driver.h"
#include "timer.h"
#include "adc.h"
#include "led.h"
#include <stdio.h>

extern volatile unsigned long Sys_Tick;
extern volatile unsigned char rx_cmd;  // Qt로부터 수신받은 명령어 ('1': 차단, '0': 복구)
extern volatile unsigned char rx_flag; // 명령어 수신 플래그

// 밸브 상태 관리 (0: 정상/개방, 1: 위험/차단)
static unsigned char valve_state = 0;

// ----------------------------------------------------
// ⭐ 1. 밸브 / 모터 / LED 제어 뼈대 함수
// ----------------------------------------------------
static void Valve_Set_State(unsigned char state)
{
    valve_state = state;

    // 1이면 LED 2개 ON, 0이면 LED 2개 OFF
    Barrier_LED_Display(state);

    if (state == 1)
    {
        // TODO: 모터 차단 구동
    }
    else
    {
        // TODO: 모터 복구 구동
    }
}

// ----------------------------------------------------
// ⭐ 2. Qt 명령어 수신 파싱 함수 ('1' / '0')
// ----------------------------------------------------
static void Process_Command(unsigned char cmd)
{
    if (cmd == '1')
    {
        Valve_Set_State(1); // 밸브 차단
    }
    else if (cmd == '0')
    {
        Valve_Set_State(0); // 밸브 복구
    }
}

static void Sys_Init(int baud)
{
    SCB->CPACR |= (0x3 << 10 * 2) | (0x3 << 11 * 2);
    Clock_Init();
    Uart2_Init(baud);
    setvbuf(stdout, NULL, _IONBF, 0);
}

void Main(void)
{
    unsigned short adc_val;
    unsigned long cur_tick = 0L;

    Sys_Init(115200);
    printf("\n=== Smart Monitoring System ===\n");

    // Timer_Init(); // WS2812B (PA7, TIM3_CH2)
    TIM4_Init();
    ADC1_Init(); // 가변저항 ADC (PA6)
    Barrier_LED_Init();

    Uart2_RX_Interrupt_Enable(1);

    // 초기 밸브 상태 설정 (정상/개방 상태)
    Valve_Set_State(0);

    printf("System Ready!\n");

    for (;;)
    {
        // [단계 1] Qt로부터 원격 제어 명령 수신 시 처리
        if (rx_flag)
        {
            Process_Command(rx_cmd);
            rx_flag = 0;
        }

        // [단계 2] 100ms 주기로 ADC 측정 및 Qt로 송신
        if ((Sys_Tick - cur_tick) < 100)
            continue;

        cur_tick = Sys_Tick;

        // ADC 값 읽기 (0 ~ 4095)
        adc_val = ADC1_Read();

        // Qt로 현재 ADC 수치 전송 ("1620\n")
        printf("%d\n", adc_val);
    }
}