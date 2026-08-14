#include "motor.h"

// ----------------------------------------------------
// ⭐ 모터 GPIO 초기화 (PC0: IN1, PC1: IN2)
// ----------------------------------------------------
void Motor_Init(void)
{
    // 1. Port-C 클럭 활성화 (AHB1ENR 비트 2)
    Macro_Set_Bit(RCC->AHB1ENR, 2);

    // 2. PC0, PC1 Output 모드(01b) 설정
    // PC0: MODER [1:0] -> 01b (pos: 0)
    // PC1: MODER [3:2] -> 01b (pos: 2)
    Macro_Write_Block(GPIOC->MODER, 0x3, 0x1, 0);
    Macro_Write_Block(GPIOC->MODER, 0x3, 0x1, 2);

    // 3. Output Type: Push-Pull (0)
    Macro_Clear_Bit(GPIOC->OTYPER, 0); // PC0
    Macro_Clear_Bit(GPIOC->OTYPER, 1); // PC1

    // 4. 초기 상태: 모터 OFF (PC0=0, PC1=0)
    Motor_Off();
}

// ----------------------------------------------------
// 모터 가동 (PC0 = 1, PC1 = 0)
// ----------------------------------------------------
void Motor_On(void)
{
    Macro_Set_Bit(GPIOC->ODR, 0);   // PC0 HIGH
    Macro_Clear_Bit(GPIOC->ODR, 1); // PC1 LOW
}

// ----------------------------------------------------
// 모터 정지 (PC0 = 0, PC1 = 0)
// ----------------------------------------------------
void Motor_Off(void)
{
    Macro_Clear_Bit(GPIOC->ODR, 0); // PC0 LOW
    Macro_Clear_Bit(GPIOC->ODR, 1); // PC1 LOW
}

// ----------------------------------------------------
// 모터 상태 제어 (1: ON, 0: OFF)
// ----------------------------------------------------
void Motor_Display(int on)
{
    if (on)
    {
        Motor_On();
    }
    else
    {
        Motor_Off();
    }
}