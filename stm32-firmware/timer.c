#include "device_driver.h"
#include "timer.h"

volatile unsigned long Sys_Tick = 0; // 시스템 틱 카운터

// 타이머 모듈 전체 초기화
void Timer_Init(void)
{
	TIM4_Init();
	TIM5_Init();
}

// TIM4 초기화 (1ms 주기 시스템 틱)
void TIM4_Init(void)
{
	Macro_Set_Bit(RCC->APB1ENR, 2U);	 // TIM4 클럭 활성화
	TIM4->CR1 = (1U << 4U) | (0U << 3U); // Down-counter, Repeat Mode
	TIM4->PSC = (unsigned int)(TIMXCLK / (double)TIM_FREQ + 0.5) - 1U;
	TIM4->ARR = (unsigned int)(TIM_1MS_PLS * 1) - 1U; // 1ms 주기 설정

	Macro_Set_Bit(TIM4->EGR, 0U);
	Macro_Clear_Bit(TIM4->SR, 0U);

	NVIC_ClearPendingIRQ(TIM4_IRQn);
	Macro_Set_Bit(TIM4->DIER, 0U); // 인터럽트 허용
	NVIC_EnableIRQ(TIM4_IRQn);

	Macro_Set_Bit(TIM4->CR1, 0U); // 타이머 시작
}

// TIM5 초기화 (모터 PWM 제어용)
void TIM5_Init(void)
{
	Macro_Set_Bit(RCC->AHB1ENR, 0U); // GPIOA 클럭 활성화
	Macro_Set_Bit(RCC->APB1ENR, 3U); // TIM5 클럭 활성화

	// PA0, PA1 핀 설정 (AF 모드 / AF02: TIM5)
	Macro_Write_Block(GPIOA->MODER, 0xFU, 0xAU, 0U);
	Macro_Write_Block(GPIOA->AFR[0], 0xFFU, 0x22U, 0U);

	TIM5->CR1 = (1U << 4U) | (0U << 3U); // Down-counter, Repeat Mode

	// PWM 주파수 설정
	TIM5->PSC = (unsigned int)(TIMXCLK / TIM5_CNT_FREQ) - 1U;
	TIM5->ARR = (unsigned int)(TIM5_CNT_FREQ / TIM5_PWM_FREQ) - 1U;

	// CH1, CH2 PWM Mode 1 및 Preload 활성화
	Macro_Write_Block(TIM5->CCMR1, 0xFFFFU, 0x6868U, 0U);

	// CH1, CH2 출력 활성화
	Macro_Set_Bit(TIM5->CCER, 0U);
	Macro_Set_Bit(TIM5->CCER, 4U);

	// 동기화 및 타이머 시작
	Macro_Set_Bit(TIM5->EGR, 0U);
	Macro_Set_Bit(TIM5->CR1, 0U);
}

// TIM5 CH1, CH2 PWM Duty 비 설정 (0~100%)
void TIM5_Set_Duty(int ch1_duty, int ch2_duty)
{
	TIM5->CCR1 = (unsigned int)(TIM5->ARR * (ch1_duty / 100.0));
	TIM5->CCR2 = (unsigned int)(TIM5->ARR * (ch2_duty / 100.0));
}
