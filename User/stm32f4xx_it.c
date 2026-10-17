/**
  ******************************************************************************
  * @file    Project/STM32F4xx_StdPeriph_Templates/stm32f4xx_it.c 
  * @author  MCD Application Team
  * @version V1.8.1
  * @date    27-January-2022
  * @brief   Main Interrupt Service Routines.
  *          This file provides template for all exceptions handler and 
  *          peripherals interrupt service routine.
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

/* Includes ------------------------------------------------------------------*/
#include "stm32f4xx_it.h"
#include "main.h"
#include "stm32f4xx_rtc.h"
#include "stm32f4xx_usart.h"
#include "stm32f4xx_dma.h"
#include "Serial.h"              // HardInt_receive_str / HardInt_uart_flag / DMA 辅助函数

/** @addtogroup Template_Project
  * @{
  */

/* Private typedef -----------------------------------------------------------*/
/* Private define ------------------------------------------------------------*/
/* Private macro -------------------------------------------------------------*/
/* Private variables ---------------------------------------------------------*/
/* Private function prototypes -----------------------------------------------*/
/* Private functions ---------------------------------------------------------*/

/******************************************************************************/
/*            Cortex-M4 Processor Exceptions Handlers                         */
/******************************************************************************/

/**
  * @brief  This function handles NMI exception.
  * @param  None
  * @retval None
  */
void NMI_Handler(void)
{
}

/**
  * @brief  This function handles Hard Fault exception.
  * @param  None
  * @retval None
  */
void HardFault_Handler(void)
{
  /* ★ 把故障原因通过串口打出来，否则「莫名重启/卡死」完全无从下手。
   *   这里只用直写寄存器的 dbg_*（不经过 printf、不依赖可能已损坏的运行时）。
   *   CFSR 各位含义（常见）：
   *     bit 25 DIVBYZERO / bit 24 UNALIGNED / bit 16 UNDEFINSTR
   *     bit 10 IMPRECISERR / bit 9 PRECISERR / bit 8 IBUSERR
   *     bit  1 DACCVIOL / bit 0 IACCVIOL
   *   HFSR bit30 FORCED = 由上面某个可配置故障升级而来；
   *   HFSR bit1 VECTTBL = 取向量失败（栈被冲烂时常见）。 */
  SCB->CFSR;                    /* 读一下确保标志已锁存 */

  dbg_puts("");
  dbg_puts("!!!!!!!! HARD FAULT !!!!!!!!");
  dbg_puts("CFSR  fault cause =");
  dbg_putnum((int32_t)SCB->CFSR);
  dbg_puts("HFSR  hardfault   =");
  dbg_putnum((int32_t)SCB->HFSR);
  dbg_puts("BFAR  bus addr    =");
  dbg_putnum((int32_t)SCB->BFAR);
  dbg_puts("MMFAR mem addr    =");
  dbg_putnum((int32_t)SCB->MMFAR);
  dbg_puts("MSP =");
  dbg_putnum((int32_t)__get_MSP());
  dbg_puts("PSP =");
  dbg_putnum((int32_t)__get_PSP());
  dbg_puts("!!!!!!!!!!!!!!!!!!!!!!!!!!!!");

  /* Go to infinite loop when Hard Fault exception occurs */
  while (1)
  {
  }
}

/**
  * @brief  This function handles Memory Manage exception.
  * @param  None
  * @retval None
  */
void MemManage_Handler(void)
{
  /* Go to infinite loop when Memory Manage exception occurs */
  while (1)
  {
  }
}

/**
  * @brief  This function handles Bus Fault exception.
  * @param  None
  * @retval None
  */
void BusFault_Handler(void)
{
  /* Go to infinite loop when Bus Fault exception occurs */
  while (1)
  {
  }
}

/**
  * @brief  This function handles Usage Fault exception.
  * @param  None
  * @retval None
  */
void UsageFault_Handler(void)
{
  /* Go to infinite loop when Usage Fault exception occurs */
  while (1)
  {
  }
}

/**
  * @brief  This function handles SVCall exception.
  * @param  None
  * @retval None
  */
//void SVC_Handler(void)
//{
//}

/**
  * @brief  This function handles Debug Monitor exception.
  * @param  None
  * @retval None
  */
void DebugMon_Handler(void)
{
}

/**
  * @brief  This function handles PendSVC exception.
  * @param  None
  * @retval None
  */
//void PendSV_Handler(void)
//{
//}

/**
  * @brief  This function handles SysTick Handler.
  * @param  None
  * @retval None
  */
//void SysTick_Handler(void)
//{
//  TimingDelay_Decrement();
//}

/******************************************************************************/
/*                 STM32F4xx Peripherals Interrupt Handlers                   */
/*  Add here the Interrupt Handler for the used peripheral(s) (PPP), for the  */
/*  available peripheral interrupt handler's name please refer to the startup */
/*  file (startup_stm32f4xx.s).                                               */
/******************************************************************************/

/**
  * @brief  This function handles PPP interrupt request.
  * @param  None
  * @retval None
  */
/*void PPP_IRQHandler(void)
{
}*/

/**
  * @brief  RTC 唤醒定时器中断（RTC WakeUp Timer，EXTI Line 22）。
  *         ★ MX_RTC_Init() 首次上电会执行 RTC_ITConfig(RTC_IT_WUT, ENABLE)
  *           并打开 NVIC 的 RTC_WKUP_IRQn。原来这里没有实现处理函数，
  *           中断一旦触发就会落到启动文件的默认死循环里，整个系统卡死。
  *           这里必须把唤醒标志和 EXTI 挂起位都清掉。
  * @param  None
  * @retval None
  */
void RTC_WKUP_IRQHandler(void)
{
  if (RTC_GetITStatus(RTC_IT_WUT) != RESET)
  {
    /* 清 RTC 唤醒中断标志 */
    RTC_ClearITPendingBit(RTC_IT_WUT);

    /* ★ 关键：RTC 唤醒走 EXTI Line 22，EXTI 挂起位不清会反复进中断 */
    EXTI_ClearITPendingBit(EXTI_Line22);

    /* TODO: 如果需要「RTC 定时唤醒」做业务，在这里加：
     *       置事件标志 / xQueueSendFromISR 通知任务 */
  }
}

/**
  * @brief  USART1 中断：用 IDLE 空闲中断 + DMA 收完一帧 UART 命令。
  *
  * ★ 这个函数在移植版里【完全不存在】，是移植时丢掉的一整块功能：
  *     原版 user_HardwareInitTask.c:58-59 做了两件事
  *         HAL_UART_Receive_DMA(&huart1,(uint8_t*)HardInt_receive_str,25);
  *         __HAL_UART_ENABLE_IT(&huart1,UART_IT_IDLE);
  *     然后靠 USART1_IRQHandler 处理 IDLE 事件、通知 MessageSendTask。
  *   移植版既没有启动 DMA 接收、也没有开 IDLE 中断、更没有这个处理函数，
  *   于是 MessageSendTask 永远阻塞在 xEventGroupWaitBits(portMAX_DELAY)，
  *   通过串口设时间 / OV+VERSION / OV+SEND 等功能全部失效。
  *
  *   而且它是个隐患：MX_USART1_UART_Init() 已经在 NVIC 里使能了 USART1 中断，
  *   一旦 IDLE 中断被打开而没有处理函数，就会落到启动文件的默认死循环。
  *
  * @param  None
  * @retval None
  */
void USART1_IRQHandler(void)
{
    /* IDLE 空闲中断：一帧数据接收结束（总线空闲）时触发 */
    if (USART_GetITStatus(USART1, USART_IT_IDLE) != RESET)
    {
        /* 清 IDLE 标志的标准做法：先读 SR 再读 DR */
        (void)USART1->SR;
        (void)USART1->DR;

        /* 停掉本次 DMA 接收，算出实际收到了多少字节，并补上字符串结束符 */
        DMA_Cmd(DMA2_Stream5, DISABLE);
        {
            uint16_t rx_len = USART1_DMA_RX_Count(25U);

            if (rx_len >= 25U)
            {
                rx_len = 24U;               /* 末字节留给 '\0' */
            }
            HardInt_receive_str[rx_len] = '\0';
        }

        /* 重新武装 DMA，等下一帧 */
        USART1_DMA_RX_Restart_ISR((uint8_t *)HardInt_receive_str, 25);

        /* ★ 只置标志位，绝不在这里调用任何 FreeRTOS API。
         *   原版就是这么做的（Core\Src\stm32f4xx_it.c:220 HardInt_uart_flag = 1）。
         *   原因：本工程 USART1 中断优先级被设为 5，数值上高于 FreeRTOS 允许
         *   调用 API 的门槛（configMAX_SYSCALL_INTERRUPT_PRIORITY），
         *   在 ISR 里调 xEventGroupSetBitsFromISR 之类的 FromISR 接口是违规的。
         *   由 MessageSendTask 轮询这个标志来消费数据。 */
        HardInt_uart_flag = 1;
    }
}

/**
  * @}
  */ 


