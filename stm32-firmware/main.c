#include "device_driver.h"
#include "timer.h"
#include "adc.h"
#include "led.h"
#include "motor.h"
#include <stdio.h>

extern volatile unsigned long Sys_Tick;
extern volatile unsigned char rx_cmd;  // Qt로부터 수신받은 명령어 ('1': 차단, '0': 복구)
extern volatile unsigned char rx_flag; // 명령어 수신 플래그

// 시스템 밸브 상태 (0: 정상/개방, 1: 위험/차단)
static unsigned char g_valve_state = 0;

// ----------------------------------------------------
// ⭐ 밸브 상태 제어 함수 (LED + 모터 환풍기 동시 제어)
// ----------------------------------------------------
static void Valve_Set_State(unsigned char state)
{
    g_valve_state = state;

    // 1: LED ON + 모터 회전 (차단/환기)
    // 0: LED OFF + 모터 정지 (정상/복구)
    Barrier_LED_Display(state);
    Motor_Display(state);
}

// ----------------------------------------------------
// Qt 명령어 수신 처리
// ----------------------------------------------------
static void Process_Command(unsigned char cmd)
{
    if (cmd == '1')
    {
        Valve_Set_State(1); // 차단 및 환기팬 가동
    }
    else if (cmd == '0')
    {
        Valve_Set_State(0); // 복구 및 환기팬 정지
    }
}

static void Sys_Init(int baud)
{
    SCB->CPACR |= (0x3 << 10 * 2) | (0x3 << 11 * 2);
    Clock_Init();
    Uart2_Init(baud);
    setvbuf(stdout, NULL, _IONBF, 0);

    Barrier_LED_Init(); // LED 초기화 (PB0, PB1)
    Motor_Init();       // 모터 초기화 (PB4, PB5)
}

void Main(void)
{
    unsigned short adc_val;
    unsigned long cur_tick = 0L;

    Sys_Init(115200);
    printf("\n=== Smart Monitoring System ===\n");

    TIM4_Init();
    ADC1_Init(); // 가변저항 ADC (PA6)
    Uart2_RX_Interrupt_Enable(1);

    // 초기 상태: 정상 (LED OFF, 모터 OFF)
    Valve_Set_State(0);

    printf("System Ready!\n");

    for (;;)
    {
        // 1. Qt 원격 제어 명령 수신
        if (rx_flag)
        {
            Process_Command(rx_cmd);
            rx_flag = 0;
        }

        // 2. 100ms 주기로 가변저항 센서값 Qt 전송
        if ((Sys_Tick - cur_tick) < 100)
            continue;

        cur_tick = Sys_Tick;

        adc_val = ADC1_Read();
        printf("%d\n", adc_val);
    }
}