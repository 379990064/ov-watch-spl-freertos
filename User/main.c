/* main.c —— 主程序（对应 HAL 参考结构的标准库 + FreeRTOS 版）
   调试输出：直接写 USART1 寄存器（绕过 printf/重定向，MicroLIB 未开也能用） */
/* Includes ------------------------------------------------------------------*/
#include "main.h"
#include "stm32f4xx.h"                  // Device header
#include "MYSPI.h"                      // SPI1（LCD）
#include "tim.h"                        // 背光 TIM2（PA15）
#include "Serial.h"                     // USART1
#include "user_TasksInit.h"             // task_init()
#include "FreeRTOS.h"
#include "task.h"
#include "MYSPI.h"

/* 直接写 USART1 的调试输出（不依赖 printf/重定向） */
static void u1_send(uint8_t c)
{
    while (USART_GetFlagStatus(USART1, USART_FLAG_TXE) == RESET);
    USART_SendData(USART1, c);
}
static void u1_puts(const char *s)
{
    while (*s) u1_send((uint8_t)*s++);
    u1_send('\r');
    u1_send('\n');
}

/* Private function prototypes -----------------------------------------------*/
void SystemClock_Config(void);

/**
 * @brief  The application entry point.
 * @retval int
 */
int main(void)
{
	MX_SPI1_Init();
	LCD_Init();
	LCD_Fill(80,80,160,160,5555);
}
