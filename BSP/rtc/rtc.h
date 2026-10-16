#ifndef __RTC_H
#define __RTC_H
#include "stm32f4xx.h"                  // Device header



void MX_RTC_Init(void);
void RTC_MspInit(void);
void RTC_MspDeInit(void);
uint8_t weekday_calculate(int y, int m, int d, int c);
void RTC_SetTime1(uint8_t hours, uint8_t minutes, uint8_t seconds);
void RTC_SetDate1(uint8_t year, uint8_t month, uint8_t date);

#endif
