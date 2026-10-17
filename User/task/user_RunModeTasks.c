/* Private includes -----------------------------------------------------------*/
//includes
#include "user_TasksInit.h"

#include "ui.h"
#include "ui_HomePage.h"
#include "ui_OffTimePage.h"
#include "ui_TimerPage.h"

#include "main.h"
#include "stm32f4xx_it.h"
#include "delay.h"            
#include "serial.h"          
#include "lcd_init.h"
#include "CST816.h"
#include "key.h"

#include "HWDataAccess.h"

#include "stm32f4xx.h"
#include "stm32f4xx_pwr.h"
#include "stm32f4xx_rcc.h"


uint32_t IdleTimerCount = 0;


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
            
        sleep:
            IdleTimerCount = 0;

            //sensors

            //usart
            MX_USART1_UART_Init();

            //lcd
            LCD_RES_Clr();
            LCD_Close_Light();

            //touch
            CST816_Sleep();


            vTaskSuspendAll();
            CLEAR_BIT(SysTick->CTRL, SysTick_CTRL_TICKINT_Msk);


            PWR_EnterSTOPMode(PWR_Regulator_ON, PWR_STOPEntry_WFI);

            //  CPU 停止 


            // 退出休眠 

            SystemClock_Config();       
            Delay_Init();              

            SET_BIT(SysTick->CTRL, SysTick_CTRL_TICKINT_Msk);

           
            xTaskResumeAll();



            // 唤醒后判断
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


            // 唤醒后恢复 

            //usart
            MX_USART1_UART_Init();

            //lcd
            LCD_Init();
            LCD_Set_Light(ui_LightSliderValue);

            //touch
            CST816_Wakeup();


        }   

        vTaskDelay(pdMS_TO_TICKS(100));
    }
}
