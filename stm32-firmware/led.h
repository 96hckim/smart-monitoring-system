#ifndef LED_H
#define LED_H

#include "device_driver.h"

/**
 * @brief 차단벽 LED GPIO 초기화 (PB0: 상단, PB1: 하단, Push-Pull)
 */
void Barrier_LED_Init(void);

/**
 * @brief 차단벽 LED 전체 점등 (PB0=HIGH, PB1=HIGH)
 */
void Barrier_LED_On(void);

/**
 * @brief 차단벽 LED 전체 소등 (PB0=LOW, PB1=LOW)
 */
void Barrier_LED_Off(void);

/**
 * @brief 차단벽 LED 상태 일괄 제어
 * @param on 1: 전체 점등, 0: 전체 소등
 */
void Barrier_LED_Display(int on);

void Barrier_LED_Top(int on);
void Barrier_LED_Bottom(int on);

#endif // LED_H