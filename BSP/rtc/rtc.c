/**
  ******************************************************************************
  * @file    rtc.c
  * @brief   RTC configuration (Standard Peripheral Library)
  ******************************************************************************
  */

/* Includes ------------------------------------------------------------------*/
#include "rtc.h"
#include "stm32f4xx_rtc.h"
#include "stm32f4xx.h"                  // Device header


/* USER CODE BEGIN 0 */

/* USER CODE END 0 */

/* ---------------------------------------------------------------------------
 * RTC 初始化
 * 说明：
 *   1. 如果备份寄存器 RTC_BKP_DR0 != 0x5050，说明是第一次上电，需要配置时间和日期
 *   2. 如果已经配置过，跳过时间和日期设置（RTC 由电池供电，时间不会丢）
 * -------------------------------------------------------------------------*/
void MX_RTC_Init(void)
{
    /* USER CODE BEGIN RTC_Init 0 */

    /* USER CODE END RTC_Init 0 */

    RTC_InitTypeDef    RTC_InitStructure;
    RTC_TimeTypeDef    sTime;
    RTC_DateTypeDef    sDate;

    /* USER CODE BEGIN RTC_Init 1 */

    /* USER CODE END RTC_Init 1 */

    /* ---------- 1. 使能 PWR 时钟，允许访问备份域 ---------- */
    RCC_APB1PeriphClockCmd(RCC_APB1Periph_PWR, ENABLE);
    PWR_BackupAccessCmd(ENABLE);

    /* ---------- 2. 选择 RTC 时钟源（LSE） ---------- */
    RCC_LSEConfig(RCC_LSE_ON);
    while (RCC_GetFlagStatus(RCC_FLAG_LSERDY) == RESET);

    RCC_RTCCLKConfig(RCC_RTCCLKSource_LSE);
    RCC_RTCCLKCmd(ENABLE);

    /* ---------- 3. 等待 RTC APB 寄存器同步 ---------- */
    RTC_WaitForSynchro();

    /* ---------- 4. 配置 RTC 异步/同步分频 ---------- */
    RTC_InitStructure.RTC_AsynchPrediv = 127;
    RTC_InitStructure.RTC_SynchPrediv  = 255;
    RTC_InitStructure.RTC_HourFormat   = RTC_HourFormat_24;
    RTC_Init(&RTC_InitStructure);

    /* ---------- 5. 判断是否第一次上电 ---------- */
    if (RTC_ReadBackupRegister(RTC_BKP_DR0) != 0x5050)
    {

        /* ---------- 5.1 设置初始时间 ---------- */
        sTime.RTC_Hours   = 0x11;
        sTime.RTC_Minutes = 0x59;
        sTime.RTC_Seconds = 0x55;
        sTime.RTC_H12     = RTC_H12_AM;
        RTC_SetTime(RTC_Format_BCD, &sTime);

        /* ---------- 5.2 设置初始日期 ---------- */
        sDate.RTC_WeekDay = RTC_Weekday_Sunday;
        sDate.RTC_Month   = RTC_Month_May;
        sDate.RTC_Date    = 0x21;
        sDate.RTC_Year    = 0x23;
        RTC_SetDate(RTC_Format_BCD, &sDate);

        /* ---------- 5.3 启动唤醒定时器中断 ---------- */
        /* 先关闭唤醒定时器 */
        RTC_WakeUpCmd(DISABLE);

        RTC_WakeUpClockConfig(RTC_WakeUpClock_RTCCLK_Div16);

        /* 计数 2000 次后唤醒（约 1 秒） */
        RTC_SetWakeUpCounter(2000);

        /* 清除唤醒标志，使能唤醒中断 */
        RTC_ClearITPendingBit(RTC_IT_WUT);
        RTC_ITConfig(RTC_IT_WUT, ENABLE);

        /* 启动唤醒定时器 */
        RTC_WakeUpCmd(ENABLE);


        /* USER CODE BEGIN RTC_Init 2 */
        /* 写入魔数，标记已初始化 */
        RTC_WriteBackupRegister(RTC_BKP_DR0, 0x5050);
    }
    else
    {
        RTC_WakeUpCmd(DISABLE);
        RTC_WakeUpClockConfig(RTC_WakeUpClock_RTCCLK_Div16);
        RTC_SetWakeUpCounter(2000);
        RTC_ClearITPendingBit(RTC_IT_WUT);
        RTC_ITConfig(RTC_IT_WUT, ENABLE);
        RTC_WakeUpCmd(ENABLE);
    }
}

/* ---------------------------------------------------------------------------
 * RTC 时钟源和中断配置（替代 HAL_RTC_MspInit）
 * -------------------------------------------------------------------------*/
void RTC_MspInit(void)
{
    /* USER CODE BEGIN RTC_MspInit 0 */

    /* USER CODE END RTC_MspInit 0 */

    /* 使能 PWR 时钟 */
    RCC_APB1PeriphClockCmd(RCC_APB1Periph_PWR, ENABLE);

    RCC_RTCCLKCmd(ENABLE);

    /* ---------- 配置 NVIC：RTC 唤醒中断 ---------- */
    NVIC_InitTypeDef NVIC_InitStructure;
    NVIC_InitStructure.NVIC_IRQChannel                   = RTC_WKUP_IRQn;
    NVIC_InitStructure.NVIC_IRQChannelPreemptionPriority = 5;
    NVIC_InitStructure.NVIC_IRQChannelSubPriority        = 0;
    NVIC_InitStructure.NVIC_IRQChannelCmd                = ENABLE;
    NVIC_Init(&NVIC_InitStructure);

    /* USER CODE BEGIN RTC_MspInit 1 */

    /* USER CODE END RTC_MspInit 1 */
}

/* ---------------------------------------------------------------------------
 * RTC 反初始化
 * -------------------------------------------------------------------------*/
void RTC_MspDeInit(void)
{
    /* USER CODE BEGIN RTC_MspDeInit 0 */

    /* USER CODE END RTC_MspDeInit 0 */

    /* 关闭 RTC 时钟 */
    RCC_RTCCLKCmd(DISABLE);

    /* 关闭 RTC 唤醒中断 */
    NVIC_InitTypeDef NVIC_InitStructure;
    NVIC_InitStructure.NVIC_IRQChannel                   = RTC_WKUP_IRQn;
    NVIC_InitStructure.NVIC_IRQChannelPreemptionPriority = 5;
    NVIC_InitStructure.NVIC_IRQChannelSubPriority        = 0;
    NVIC_InitStructure.NVIC_IRQChannelCmd                = DISABLE;
    NVIC_Init(&NVIC_InitStructure);

}


/* ---------------------------------------------------------------------------
 * 星期计算：基姆拉尔森公式
 * -------------------------------------------------------------------------*/
uint8_t weekday_calculate(int y, int m, int d, int c)
{
    int w;
    if (m == 1 || m == 2)
    {
        y--;
        m += 12;
    }
    w = y + y / 4 + c / 4 + 26 * (m + 1) / 10 + d - 1 - 2 * c;
    while (w < 0)
        w += 7;
    w %= 7;
    w = (w == 0) ? 7 : w;
    return w;
}

/* ---------------------------------------------------------------------------
 * 设置时间（封装函数，给上层调用）
 * -------------------------------------------------------------------------*/
void RTC_SetTime1(uint8_t hours, uint8_t minutes, uint8_t seconds)
{
    RTC_TimeTypeDef time = {0};

    time.RTC_Hours   = hours;
    time.RTC_Minutes = minutes;
    time.RTC_Seconds = seconds;
    time.RTC_H12     = RTC_H12_AM;

    RTC_SetTime(RTC_Format_BIN, &time);
}

/* ---------------------------------------------------------------------------
 * 设置日期（封装函数，给上层调用）
 * -------------------------------------------------------------------------*/
void RTC_SetDate1(uint8_t year, uint8_t month, uint8_t date)
{
    RTC_DateTypeDef setdate = {0};


    setdate.RTC_Date    = date;
    setdate.RTC_Month   = month;
    setdate.RTC_Year    = year;
    setdate.RTC_WeekDay = weekday_calculate(year, month, date, 20);


    RTC_SetDate(RTC_Format_BIN, &setdate);
}


