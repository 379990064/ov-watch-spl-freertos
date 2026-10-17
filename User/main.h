/**
  ******************************************************************************
  * @file    Project/STM32F4xx_StdPeriph_Templates/main.h 
  * @author  MCD Application Team
  * @version V1.8.1
  * @date    27-January-2022
  * @brief   Header for main.c module
  ******************************************************************************
  * @attention
  *
  * Copyright (c) 2016 STMicroelectronics.
  * All rights reserved.
  *
  * This software is licensed under terms that can be found in the LICENSE file
  * in the root directory of this software component.
  * If no LICENSE file comes with this software, it is provided AS-IS.
  *
  ******************************************************************************
  */
  
/* Define to prevent recursive inclusion -------------------------------------*/
#ifndef __MAIN_H
#define __MAIN_H

/* Includes ------------------------------------------------------------------*/
#include "stm32f4xx.h"


/* Exported types ------------------------------------------------------------*/
/* Exported constants --------------------------------------------------------*/
/* Exported macro ------------------------------------------------------------*/
/* Exported functions ------------------------------------------------------- */
void TimingDelay_Decrement(void);

/* ★ 系统时钟配置：定义在 User\main.c。
 *   休眠唤醒流程（user_RunModeTasks.c 的 StopEnterTask）在退出 STOP 模式后
 *   必须调用它把时钟重新配回 100MHz —— 否则 F4 会停在 HSI 16MHz，
 *   而 configCPU_CLOCK_HZ 仍写着 100MHz，系统节拍会变成 6.25ms 一次。 */
void SystemClock_Config(void);

#endif /* __MAIN_H */

