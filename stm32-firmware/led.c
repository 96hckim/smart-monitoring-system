#include "led.h"

// ----------------------------------------------------
// ⭐ 차단벽 LED 초기화 (PB0, PB1)
// ----------------------------------------------------
void Barrier_LED_Init(void)
{
	// 1. Port-B Clock Enable (AHB1ENR 비트 1)
	Macro_Set_Bit(RCC->AHB1ENR, 1);

	// 2. PB0 Output 설정 (MODER [1:0] -> 01b)
	Macro_Write_Block(GPIOB->MODER, 0x3, 0x1, 0);

	// 3. PB1 Output 설정 (MODER [3:2] -> 01b)
	Macro_Write_Block(GPIOB->MODER, 0x3, 0x1, 2);

	// 4. Output Type: Push-Pull (0)
	Macro_Clear_Bit(GPIOB->OTYPER, 0); // PB0
	Macro_Clear_Bit(GPIOB->OTYPER, 1); // PB1

	// 5. 초기 상태: 소등 (OFF)
	Macro_Clear_Bit(GPIOB->ODR, 0); // PB0 OFF
	Macro_Clear_Bit(GPIOB->ODR, 1); // PB1 OFF
}

// ----------------------------------------------------
// ⭐ 차단벽 전체 ON (PB0, PB1 동시 점등)
// ----------------------------------------------------
void Barrier_LED_On(void)
{
	Macro_Set_Bit(GPIOB->ODR, 0);
	Macro_Set_Bit(GPIOB->ODR, 1);
}

// ----------------------------------------------------
// ⭐ 차단벽 전체 OFF (PB0, PB1 동시 소등)
// ----------------------------------------------------
void Barrier_LED_Off(void)
{
	Macro_Clear_Bit(GPIOB->ODR, 0);
	Macro_Clear_Bit(GPIOB->ODR, 1);
}

// ----------------------------------------------------
// ⭐ 차단벽 동시 제어 (1: ON, 0: OFF)
// ----------------------------------------------------
void Barrier_LED_Display(int on)
{
	// on이 1이면 0x3(11b) 작성하여 둘 다 ON, 0이면 0x0 작성하여 둘 다 OFF
	Macro_Write_Block(GPIOB->ODR, 0x3, on ? 0x3 : 0x0, 0);
}

// ----------------------------------------------------
// 개별 LED 제어 (필요 시)
// ----------------------------------------------------
void Barrier_LED_Top(int on)
{
	Macro_Write_Block(GPIOB->ODR, 0x1, on & 0x1, 0); // PB0 제어
}

void Barrier_LED_Bottom(int on)
{
	Macro_Write_Block(GPIOB->ODR, 0x1, on & 0x1, 1); // PB1 제어
}
