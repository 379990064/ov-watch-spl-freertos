/* Private includes -----------------------------------------------------------*/
//includes
#include "user_TasksInit.h"

#include "ui.h"
#include "ui_HomePage.h"
#include "ui_OffTimePage.h"
#include "ui_TimerPage.h"

#include "main.h"
#include "stm32f4xx_it.h"
#include "serial.h"           /* ★ 里面有 serial_MspInit / serial_MspDeInit */
#include "lcd_init.h"
#include "CST816.h"
#include "key.h"

#include "HWDataAccess.h"

/* ★ SPL 头文件 */
#include "stm32f4xx.h"
#include "stm32f4xx_pwr.h"
#include "stm32f4xx_rcc.h"

/* Private typedef -----------------------------------------------------------*/

/* Private define ------------------------------------------------------------*/

/* Private variables ---------------------------------------------------------*/
uint32_t IdleTimerCount = 0;

/* Private function prototypes -----------------------------------------------*/

/* Tasks ---------------------------------------------------------------------*/

/**
  * @brief  Enter Idle state
  * @param  argument: Not used
  * @retval None
  */
void IdleEnterTask(void *argument)
{
    uint8_t Idlestr = 0;
    uint8_t IdleBreakstr = 0;
    uint8_t light_off_sent = 0;

    while (1)
    {
        if (xQueueReceive(Idle_MessageQueue, &Idlestr, pdMS_TO_TICKS(1)) == pdTRUE)
        {
            LCD_Set_Light(5);
            light_off_sent = 1;
        }

        if (xQueueReceive(IdleBreak_MessageQueue, &IdleBreakstr, pdMS_TO_TICKS(1)) == pdTRUE)
        {
            IdleTimerCount = 0;
            light_off_sent = 0;
            LCD_Set_Light(ui_LightSliderValue);
        }

        if ((!light_off_sent) && (IdleTimerCount >= ((uint32_t)ui_LTimeValue * 1000U)))
        {
            uint8_t IdleMsg = 0;
            xQueueSend(Idle_MessageQueue, &IdleMsg, 0);
        }

        if (IdleTimerCount >= ((uint32_t)ui_TTimeValue * 1000U))
        {
            uint8_t Stopstr = 1;
            IdleTimerCount = 0;
            light_off_sent = 0;
            xQueueSend(Stop_MessageQueue, &Stopstr, 0);
        }

        vTaskDelay(pdMS_TO_TICKS(10));
    }
}

/**
  * @brief  enter the stop mode and resume
  * @param  argument: Not used
  * @retval None
  */
void StopEnterTask(void *argument)
{
    uint8_t Stopstr;
    uint8_t HomeUpdataStr = 0;
    uint8_t Wrist_Flag = 0;

    while (1)
    {
        if (xQueueReceive(Stop_MessageQueue, &Stopstr, 0) == pdTRUE)
        {
            /*************************** 休眠前操作 ***************************/
        sleep:
            IdleTimerCount = 0;

            //sensors

            //usart（★ SPL 版：自己写的反初始化）
            MX_USART1_UART_Init();

            //lcd
            LCD_RES_Clr();
            LCD_Close_Light();

            //touch
            CST816_Sleep();

            /*******************************************************************/

            /****************************** 进入休眠 ******************************/

            vTaskSuspendAll();
            CLEAR_BIT(SysTick->CTRL, SysTick_CTRL_TICKINT_Msk);

            /* ★ SPL 版 STOP 模式 */
            PWR_EnterSTOPMode(PWR_Regulator_ON, PWR_STOPEntry_WFI);

            // ============ 这里是休眠期，CPU 停止 ============

            /*******************************************************************/

            /****************************** 退出休眠 ******************************/

            SET_BIT(SysTick->CTRL, SysTick_CTRL_TICKINT_Msk);
            /* ★ SPL / CMSIS 版 SysTick（建议加回） */
            SysTick_Config(SystemCoreClock);
            xTaskResumeAll();

            /*******************************************************************/

            /****************************** 唤醒后判断 ******************************/

            /* MPU 手腕检测 */
            if (HWInterface.IMU.wrist_is_enabled)
            {
                uint8_t hor;
                hor = 0;
                if (hor && HWInterface.IMU.wrist_state == WRIST_DOWN)
                {
                    HWInterface.IMU.wrist_state = WRIST_UP;
                    Wrist_Flag = 1;
                }
                else if (!hor && HWInterface.IMU.wrist_state == WRIST_UP)
                {
                    HWInterface.IMU.wrist_state = WRIST_DOWN;
                    IdleTimerCount = 0;
                    goto sleep;
                }
            }

            /* 有效唤醒源：按键、抬腕 */
            if (!KEY1 || Wrist_Flag)
            {
                Wrist_Flag = 0;
            }
            else
            {
                IdleTimerCount = 0;
                goto sleep;
            }

            /*******************************************************************/

            /****************************** 唤醒后恢复 ******************************/

            //usart（★ SPL 版：自己写的初始化）
            MX_USART1_UART_Init();

            //lcd
            LCD_Init();
            LCD_Set_Light(ui_LightSliderValue);

            //touch
            CST816_Wakeup();

            /* 检查是否在充电（★ FreeRTOS 事件组） */

        }   /* ★ 注意：原代码这里有个大括号位置问题，见下文 */

        vTaskDelay(pdMS_TO_TICKS(100));
    }
}
