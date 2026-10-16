#ifndef __KEY_H
#define __KEY_H
#include "stm32f4xx.h"                  // Device header

#define KEY1        GPIO_ReadInputDataBit(GPIOA, GPIO_Pin_0)

void Key_Init(void);
uint8_t Key_GetNum(uint8_t mode);
uint8_t KeyScan(uint8_t mode);

#endif
