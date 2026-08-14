#ifndef MOTOR_H
#define MOTOR_H

#include "device_driver.h"

// 모터 초기화 및 제어 함수 원형
void Motor_Init(void);
void Motor_On(void);
void Motor_Off(void);
void Motor_Display(int on); // 1: 켜짐(회전), 0: 꺼짐(정지)

#endif // MOTOR_H