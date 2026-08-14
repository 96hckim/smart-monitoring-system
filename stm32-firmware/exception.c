#include "device_driver.h"
#include <stdio.h>

void _Invalid_ISR(void)
{
	unsigned int r = Macro_Extract_Area(SCB->ICSR, 0x1ff, 0);
	printf("\nInvalid_Exception: %d!\n", r);
	printf("Invalid_ISR: %d!\n", r - 16);
	for (;;)
		;
}

extern volatile unsigned long Sys_Tick;

// TIM4 ISR
void TIM4_IRQHandler(void)
{
	Macro_Clear_Bit(TIM4->SR, 0U);
	NVIC_ClearPendingIRQ(TIM4_IRQn);

	Sys_Tick++; // 1ms 시스템 틱 증가
}

#define RX_BUF_SIZE 32

volatile unsigned char rx_cmd = '0';
volatile unsigned char rx_flag = 0;

void USART2_IRQHandler(void)
{
	rx_cmd = (unsigned char)(USART2->DR & 0xFF);
	rx_flag = 1;
}
