#include "device_driver.h"
#include "timer.h"
#include "adc.h"
#include <stdio.h>

static void Sys_Init(int baud)
{
    SCB->CPACR |= (0x3 << 10 * 2) | (0x3 << 11 * 2);
    Clock_Init();
    Uart2_Init(baud);
    setvbuf(stdout, NULL, _IONBF, 0);
    LED_Init();
}

extern volatile unsigned long Sys_Tick;
extern volatile unsigned char rx_cmd; // 1: 차단, 0: 개방/복구
extern volatile unsigned char rx_flag;

void Main(void)
{
    unsigned short adc_val;
    unsigned long cur_tick = 0L;

    Sys_Init(115200);
    printf("\n=== Smart Monitoring System ===\n");

    // Timer_Init(); // WS2812B (PA7, TIM3_CH2)
    TIM4_Init();
    ADC1_Init(); // 가변저항 ADC (PA6)

    Uart2_RX_Interrupt_Enable(1);

    printf("System Ready!\n");

    for (;;)
    {
        if (rx_flag)
        {
            printf("%c\n", rx_cmd);
            rx_flag = 0;
        }

        if ((Sys_Tick - cur_tick) < 100)
            continue;

        cur_tick = Sys_Tick;

        // ADC 값 읽기 (0 ~ 4095)
        adc_val = ADC1_Read();

        printf("%d\n", adc_val);
    }
}