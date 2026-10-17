/*
 * FreeRTOS V202107.00
 * Copyright (C) 2020 Amazon.com, Inc. or its affiliates.  All Rights Reserved.
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy of
 * this software and associated documentation files (the "Software"), to deal in
 * the Software without restriction, including without limitation the rights to
 * use, copy, modify, merge, publish, distribute, sublicense, and/or sell copies of
 * the Software, and to permit persons to whom the Software is furnished to do so,
 * subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in all
 * copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, FITNESS
 * FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR
 * COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER
 * IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN
 * CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
 *
 * http://www.FreeRTOS.org
 * http://aws.amazon.com/freertos
 *
 * 1 tab == 4 spaces!
 */

#ifndef FREERTOS_CONFIG_H
#define FREERTOS_CONFIG_H

/*-----------------------------------------------------------
 * Application specific definitions.
 *
 * These definitions should be adjusted for your particular hardware and
 * application requirements.
 *
 * THESE PARAMETERS ARE DESCRIBED WITHIN THE 'CONFIGURATION' SECTION OF THE
 * FreeRTOS API DOCUMENTATION AVAILABLE ON THE FreeRTOS.org WEB SITE. 
 *
 * See http://www.freertos.org/a00110.html
 *----------------------------------------------------------*/

#define configUSE_PREEMPTION		1
#define configUSE_IDLE_HOOK			0
#define configUSE_TICK_HOOK			0
/* ★ configCPU_CLOCK_HZ 必须等于实际 HCLK（见 main.c 的 SystemClock_Config）。
 *   FreeRTOS 的 SysTick 重载值 = configSYSTICK_CLOCK_HZ / configTICK_RATE_HZ，
 *   这里原来是 72MHz，而实际跑的是 100MHz，
 *   会让所有 vTaskDelay / pdMS_TO_TICKS 都变成实际时间的 1.39 倍。
 *   原版用的是 ( SystemCoreClock )，但 port.c 里没有包含 CMSIS 的
 *   system_stm32f4xx.h，直接用会报 "SystemCoreClock is undefined"，
 *   所以这里写显式数值。改主频时【必须】同步改这一行！ */
#define configCPU_CLOCK_HZ			( ( unsigned long ) 100000000 )	
#define configTICK_RATE_HZ			( ( TickType_t ) 1000 )
#define configMAX_PRIORITIES		( 20 )
#define configMINIMAL_STACK_SIZE	( ( unsigned short ) 128 )
/* ★ 原来 17KB 不够：下面 task_init() 创建的 7 个任务光 TCB+栈就要 17700 字节，
 *   堆不够时 xTaskCreate 返回失败、任务根本没建出来（默认不报错），
 *   屏幕就一直停在 lv_init() 之后、ui_init() 之前，什么都不显示。
 *   现改为 30KB，STM32F411RE 有 128KB SRAM，放得下。 */
#define configTOTAL_HEAP_SIZE		( ( size_t ) ( 30 * 1024 ) )
#define configMAX_TASK_NAME_LEN		( 16 )
#define configUSE_TRACE_FACILITY	0
#define configUSE_16_BIT_TICKS		0
#define configIDLE_SHOULD_YIELD		1

/* ★ 软件定时器：【故意不启用】
 *   原版用 osTimerNew 做 100ms 空闲计时。但启用软件定时器要额外创建
 *   定时器服务任务 + 定时器队列，都要从本就紧张的 ucHeap 里分配，
 *   一旦分配失败，vTaskStartScheduler() 会直接放弃启动。
 *   本工程改在 TaskTickHook()（1kHz 系统节拍）里累加空闲计数，
 *   零额外堆开销，效果等价。详见 User\task\user_TasksInit.c。 */
#define configUSE_TIMERS				0

/* Co-routine definitions. */
#define configUSE_CO_ROUTINES 		0
#define configMAX_CO_ROUTINE_PRIORITIES ( 2 )

/* Set the following definitions to 1 to include the API function, or zero
to exclude the API function. */

#define INCLUDE_vTaskPrioritySet		1
#define INCLUDE_uxTaskPriorityGet		1
#define INCLUDE_vTaskDelete				1
#define INCLUDE_vTaskCleanUpResources	0
#define INCLUDE_vTaskSuspend			1
#define INCLUDE_vTaskDelayUntil			1
#define INCLUDE_vTaskDelay				1
/* 诊断用：可查询任务剩余栈（单位：字），排查栈溢出 */
#define INCLUDE_uxTaskGetStackHighWaterMark	1

/* This is the raw value as per the Cortex-M3 NVIC.  Values can be 255
(lowest) to 0 (1?) (highest). */
#define configKERNEL_INTERRUPT_PRIORITY 		255
/* !!!! configMAX_SYSCALL_INTERRUPT_PRIORITY must not be set to zero !!!!
See http://www.FreeRTOS.org/RTOS-Cortex-M3-M4.html. */
#define configMAX_SYSCALL_INTERRUPT_PRIORITY 	191 /* equivalent to 0xb0, or priority 11. */


/* This is the value being used as per the ST library which permits 16
priority values, 0 to 15.  This must correspond to the
configKERNEL_INTERRUPT_PRIORITY setting.  Here 15 corresponds to the lowest
NVIC value of 255. */
#define configLIBRARY_KERNEL_INTERRUPT_PRIORITY	15

#define xPortPendSVHandler PendSV_Handler
#define vPortSVCHandler SVC_Handler
#define xPortSysTickHandler SysTick_Handler

/* ★ configASSERT —— 原版有定义，移植版漏了，这是很多问题长期被掩盖的原因：
 *   没有它，堆分配失败（xTaskCreate / xQueueCreate 返回 NULL）、
 *   优先级越界、API 误用全都是【静默】的。
 *   （任务栈 4 倍膨胀导致 ScrRenewTask 创建失败却毫无提示，就是被它掩盖的。）
 *   这里定义一个能被调试器抓住的断言：断点死循环。 */
void vAssertCalled(const char *pcFile, unsigned long ulLine);
#define configASSERT( x ) \
    if( ( x ) == 0 ) { vAssertCalled( __FILE__, __LINE__ ); }


#endif /* FREERTOS_CONFIG_H */

