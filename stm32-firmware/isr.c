#include "device_driver.h"
#include <stdio.h>

extern volatile unsigned long Sys_Tick;

volatile unsigned char rx_cmd = '0';
volatile unsigned char rx_flag = 0;

void _Invalid_ISR(void)
{
	unsigned int r = Macro_Extract_Area(SCB->ICSR, 0x1ff, 0);
	printf("\nInvalid_Exception: %d!\n", r);
	printf("Invalid_ISR: %d!\n", r - 16);
	for (;;)
		;
}

// 1ms 주기 시스템 틱 ISR
void TIM4_IRQHandler(void)
{
	Macro_Clear_Bit(TIM4->SR, 0U);
	NVIC_ClearPendingIRQ(TIM4_IRQn);
	Sys_Tick++;
}

// UART2 수신 인터럽트 ISR (Qt 원격 제어 명령 수신)
void USART2_IRQHandler(void)
{
	rx_cmd = (unsigned char)(USART2->DR & 0xFF);
	rx_flag = 1;
}