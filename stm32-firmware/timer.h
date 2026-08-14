#ifndef __TIMER_H__
#define __TIMER_H__

#define TIM_TICK (20U)                  // 타이머 틱 단위 (us)
#define TIM_FREQ (1000000.0 / TIM_TICK) // 카운트 주파수 (Hz)
#define TIM_1MS_PLS (TIM_FREQ / 1000.0) // 1ms당 카운트 펄스 수
#define TIM_MAX_PLS (0xFFFFU)

#define TIM5_PWM_FREQ (20000U) // PWM 목표 주파수 (20kHz)
#define TIM5_CNT_FREQ (TIM5_PWM_FREQ * 100U)

void Timer_Init(void);
void TIM4_Init(void); // 1ms 시스템 틱 생성 타이머

#endif // __TIMER_H__