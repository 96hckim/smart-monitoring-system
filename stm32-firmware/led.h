#ifndef LED_H
#define LED_H

#include "device_driver.h"

// ----------------------------------------------------
// 차단벽 LED 함수 원형 선언 (PB0: 상단, PB1: 하단)
// ----------------------------------------------------
void Barrier_LED_Init(void);
void Barrier_LED_On(void);
void Barrier_LED_Off(void);
void Barrier_LED_Display(int on);

// 개별 제어 함수
void Barrier_LED_Top(int on);    // PB0 제어
void Barrier_LED_Bottom(int on); // PB1 제어

#endif // LED_H