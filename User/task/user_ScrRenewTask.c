/* Private includes -----------------------------------------------------------*/
//includes
#include "user_TasksInit.h"
#include "user_ScrRenewTask.h"
#include "main.h"
#include "lvgl.h"
#include "ui_HomePage.h"
#include "ui_MenuPage.h"

/* ★ FreeRTOS 原生头文件（一般 user_TasksInit.h 已经包含了，
      但为了明确，这里也可以显式加） */
#include "FreeRTOS.h"
#include "task.h"
#include "queue.h"

/* ★ SPL 头文件（如果这个文件直接调 SPL 才需要，实际上不需要） */
/* #include "stm32f4xx.h" */

/* Private typedef -----------------------------------------------------------*/

/* Private define ------------------------------------------------------------*/

/* Private variables ---------------------------------------------------------*/
/* ★ Key_MessageQueue 已经通过 user_TasksInit.h 声明为 QueueHandle_t，
      不需要再在本地 extern，但如果你非要写，就写成： */
// extern QueueHandle_t Key_MessageQueue;

/* Private function prototypes -----------------------------------------------*/


/**
  * @brief  Screen renew task
  * @param  argument: Not used
  * @retval None
  */
void ScrRenewTask(void *argument)
{
    uint8_t keystr = 0;

    while (1)
    {
        /* ★ CMSIS: osMessageQueueGet(q, &d, NULL, 0) == osOK
           ★ FreeRTOS: xQueueReceive(q, &d, 0) == pdTRUE */
        if (xQueueReceive(Key_MessageQueue, &keystr, 0) == pdTRUE)
        {
            /* ---------- key1 短按：返回上一页 ---------- */
            if (keystr == 1)
            {
                Page_Back();
            }
            /* ---------- key2 长按：返回底层页 ---------- */
            else if (keystr == 2)
            {
                Page_Back_Bottom();
            }
        }

        /* ★ CMSIS: osDelay(10)  →  FreeRTOS: vTaskDelay(pdMS_TO_TICKS(10)) */
        vTaskDelay(pdMS_TO_TICKS(10));
    }
}
