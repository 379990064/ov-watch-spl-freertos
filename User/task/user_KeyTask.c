/* Private includes -----------------------------------------------------------*/
//includes
#include "user_TasksInit.h"
#include "ui_HomePage.h"
#include "main.h"
#include "key.h"

/* ★ FreeRTOS 原生头文件（一般 user_TasksInit.h 已包含，这里显式加更清晰） */
#include "FreeRTOS.h"
#include "task.h"
#include "queue.h"

/* Private typedef -----------------------------------------------------------*/

/* Private define ------------------------------------------------------------*/

/* Private variables ---------------------------------------------------------*/

/* Private function prototypes -----------------------------------------------*/


/**
  * @brief  Key press check task
  * @param  argument: Not used
  * @retval None
  */
void KeyTask(void *argument)
{
    uint8_t keystr = 0;
    uint8_t Stopstr = 0;
    uint8_t IdleBreakstr = 0;

    while (1)
    {
        switch (KeyScan(0))
        {
            /* ---------- 短按 ---------- */
            case 1:
                keystr = 1;
                /* ★ CMSIS: osMessageQueuePut(q, &d, 0, 1)
                   ★ FreeRTOS: xQueueSend(q, &d, pdMS_TO_TICKS(1)) */
                xQueueSend(Key_MessageQueue, &keystr, pdMS_TO_TICKS(1));
                xQueueSend(IdleBreak_MessageQueue, &IdleBreakstr, pdMS_TO_TICKS(1));
                break;

            /* ---------- 长按 ---------- */
            case 2:
                if (Page_Get_NowPage()->page_obj == &ui_HomePage)
                {
                    /* 主页长按 → 进入休眠 */
                    xQueueSend(Stop_MessageQueue, &Stopstr, pdMS_TO_TICKS(1));
                }
                else
                {
                    /* 其他页长按 → 返回上一页 */
                    keystr = 2;
                    xQueueSend(Key_MessageQueue, &keystr, pdMS_TO_TICKS(1));
                    xQueueSend(IdleBreak_MessageQueue, &IdleBreakstr, pdMS_TO_TICKS(1));
                }
                break;

            default:
                /* 无按键，什么都不做 */
                break;
        }

        /* ★ CMSIS: osDelay(1)  →  FreeRTOS: vTaskDelay(pdMS_TO_TICKS(1)) */
        vTaskDelay(pdMS_TO_TICKS(1));
    }
}