/* Private includes -----------------------------------------------------------*/
// sys
#include "Serial.h"              // 标准库工程里串口叫 Serial.h
#include "tim.h"
#include "delay.h"
// user
#include "user_TasksInit.h"
// bsp
#include "lcd.h"
#include "lcd_init.h"
#include "CST816.h"



void HardwareInitTask(void *argument)
{
    dbg_puts("[TASK] HardwareInitTask: nothing to do (init moved to main)");

    /* 直接退出并把本任务删掉；main() 已经完成了全部硬件初始化 */
    vTaskDelete(NULL);

    /* 不会执行到这里 */
    while (1)
    {
        vTaskDelay(pdMS_TO_TICKS(1000));
    }
}
