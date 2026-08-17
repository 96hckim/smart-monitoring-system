#include "device_driver.h"
#include "timer.h"

volatile unsigned long g_sys_tick = 0;

void Timer_Init(void)
{
	TIM4_Init();
}

// 1ms 주기 시스템 틱 타이머 초기화 (TIM4)
void TIM4_Init(void)
{
	Macro_Set_Bit(RCC->APB1ENR, 2U);	 // TIM4 클럭 인에이블
	TIM4->CR1 = (1U << 4U) | (0U << 3U); // Down-counter, Repeat 모드
	TIM4->PSC = (unsigned int)(TIMXCLK / (double)TIM_FREQ + 0.5) - 1U;
	TIM4->ARR = (unsigned int)(TIM_1MS_PLS * 1) - 1U; // 1ms 주기 설정

	Macro_Set_Bit(TIM4->EGR, 0U);
	Macro_Clear_Bit(TIM4->SR, 0U);

	NVIC_ClearPendingIRQ(TIM4_IRQn);
	Macro_Set_Bit(TIM4->DIER, 0U); // 인터럽트 허용
	NVIC_EnableIRQ(TIM4_IRQn);

	Macro_Set_Bit(TIM4->CR1, 0U); // 타이머 시작
}