#ifndef __TIM_H__
#define __TIM_H__

#ifdef __cplusplus
extern "C" {
#endif

#include "stm32f4xx.h"

#define PWM_PERIOD 300
#define PWM_COMPARE 0
void MX_TIM2_Init(void);
void TIM2_SetPulse(uint16_t pulse);

#ifdef __cplusplus
}
#endif

#endif /* __TIM_H__ */
