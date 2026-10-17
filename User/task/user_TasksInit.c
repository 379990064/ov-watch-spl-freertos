#include "FreeRTOS.h"
#include "task.h"
#include "queue.h"
#include "timers.h"
#include "event_groups.h"
//sys
#include "sys.h"
#include "stdio.h"
#include "lcd.h"
#include "lcd_init.h"       // LCD_Set_Light
#include "Serial.h"         // dbg_puts / dbg_putnum（调试输出，不走 printf）
#include "key.h"            // Key_Init()
#include "CST816.h"         // CST816_Get_ChipID / CST816_Get_FingerNum（诊断用）
//gui
#include "lvgl.h"
#include "ui_TimerPage.h"
#include "ui_HomePage.h"    // ui_LightSliderValue（诊断用）
//tasks
#include "user_TasksInit.h"
#include "user_HardwareInitTask.h"
#include "user_RunModeTasks.h"   // IdleTimerCount（空闲计时回调要累加它）

/* ★ 补回原版 TaskTickHook 里漏掉的一行（原版 user_TasksInit.c:238）：
 *   user_HR_timecount 是心率算法的采样时基，原版在这里每毫秒加一，
 *   供 user_SensUpdateTask.c:98 的 HR_Calculate() 使用。
 *   注意：它的原定义在 User\task\user_SensUpdateTask.c，而那个文件
 *   【不在本工程编译列表里】（HW_USE_HARDWARE=0，传感器全部裁掉），
 *   所以这里自己提供定义，等以后把传感器任务加回工程时，
 *   请把这里改成 extern 引用 user_SensUpdateTask.c 里的定义。 */
uint32_t user_HR_timecount = 0;

/**
  * @brief  FreeRTOS 断言失败处理（配合 FreeRTOSConfig.h 里的 configASSERT）
  *         停在这里，方便调试器直接看到出错的文件和行号，
  *         而不是让分配失败静默地毁掉整个系统。
  */
void vAssertCalled(const char *pcFile, unsigned long ulLine)
{
    (void)pcFile;
    (void)ulLine;
    taskDISABLE_INTERRUPTS();
    for (;;)
    {
        /* 调试时在这里下断点，然后看 pcFile / ulLine 参数 */
    }
}
#include "user_KeyTask.h"
#include "user_ScrRenewTask.h"
#include "user_MessageSendTask.h"
xTimerHandle IDLETimerHandle;
static uint16_t s_idle_tick_div = 0;

TaskHandle_t HardwareInitTaskHandle;
TaskHandle_t LvHandlerTaskHandle;
TaskHandle_t IdleEnterTaskHandle;
TaskHandle_t StopEnterTaskHandle;
TaskHandle_t KeyTaskHandle;
TaskHandle_t ScrRenewTaskHandle;
TaskHandle_t MessageSendTaskHandle;

QueueHandle_t  Key_MessageQueue;
QueueHandle_t  Idle_MessageQueue;
QueueHandle_t  Stop_MessageQueue;
QueueHandle_t  IdleBreak_MessageQueue;
QueueHandle_t  HomeUpdata_MessageQueue;
QueueHandle_t  DataSave_MessageQueue;

/* 硬件事件组 */
EventGroupHandle_t HardIntEventHandle;

void task_init(void)
{
    Key_MessageQueue      = xQueueCreate(1, 1);
    Idle_MessageQueue     = xQueueCreate(1, 1);
    Stop_MessageQueue     = xQueueCreate(1, 1);
    IdleBreak_MessageQueue= xQueueCreate(1, 1);
    HomeUpdata_MessageQueue = xQueueCreate(1, 1);
    DataSave_MessageQueue = xQueueCreate(2, 1);

    /* 创建硬件事件组（UART 事件由中断置位） */
    HardIntEventHandle = xEventGroupCreate();

    Key_Init();


xTaskCreate(
    HardwareInitTask,          
    "HardwareInitTask",        
    128 * 10 / 4,              
    NULL,                      
    configMAX_PRIORITIES - 1,
    &HardwareInitTaskHandle    
);

xTaskCreate(
    LvHandlerTask,
    "LvHandlerTask",
    128 * 24 / 4,              
    NULL,
    8,
    &LvHandlerTaskHandle
);

xTaskCreate(
    IdleEnterTask,
    "IdleEnterTask",
    128 * 1 / 4,               
    16,
    &IdleEnterTaskHandle
);

xTaskCreate(
    StopEnterTask,
    "StopEnterTask",
    128 * 16 / 4,              
    NULL,
    17,
    &StopEnterTaskHandle
);

xTaskCreate(
    KeyTask,
    "KeyTask",
    128 * 1 / 4,             
    NULL,
    13,
    &KeyTaskHandle
);

xTaskCreate(
    ScrRenewTask,
    "ScrRenewTask",
    128 * 10 / 4,           
    NULL,
    10,
    &ScrRenewTaskHandle
);

xTaskCreate(
    MessageSendTask,
    "MessageSendTask",
    128 * 4 / 4,             
    NULL,
    10,
    &MessageSendTaskHandle
);
}

void TaskTickHook(void)
{
    BaseType_t xHigherPriorityTaskWoken = pdFALSE;

    // 给 LVGL 提供 1ms 时基
    lv_tick_inc(1);

    // 计时器页面逻辑
    if (ui_TimerPageFlag)
    {
        ui_TimerPage_ms += 1;
        if (ui_TimerPage_ms >= 10)
        {
            ui_TimerPage_ms = 0;
            ui_TimerPage_10ms += 1;
        }
        if (ui_TimerPage_10ms >= 100)
        {
            ui_TimerPage_10ms = 0;
            ui_TimerPage_sec += 1;

            uint8_t IdleBreakstr = 0;
            // ★ 关键：用 FromISR 版本
            xQueueSendFromISR(IdleBreak_MessageQueue,
                              &IdleBreakstr,
                              &xHigherPriorityTaskWoken);
        }
        if (ui_TimerPage_sec >= 60)
        {
            ui_TimerPage_sec = 0;
            ui_TimerPage_min += 1;
        }
        if (ui_TimerPage_min >= 60)
        {
            ui_TimerPage_min = 0;
        }
    }



    if (++s_idle_tick_div >= 100)
    {
        s_idle_tick_div = 0;
        IdleTimerCount += 100;          /* 单位：毫秒 */
    }

    user_HR_timecount += 1;

    //  如果唤醒了更高优先级任务，请求切换
    portYIELD_FROM_ISR(xHigherPriorityTaskWoken);
}
/**
  * @brief  LVGL Handler task, to run the lvgl
  * @param  argument: Not used
  * @retval None
  */
void LvHandlerTask(void *argument)
{
    uint8_t IdleBreakstr = 0;
    uint32_t tick = 0;
    uint8_t  last_f = 0xFF;
    uint32_t edge_cnt = 0;

    while (1)
    {
        /* 最近 1000ms 内有用户操作 重置空闲计时，保持亮屏 */
        if (lv_disp_get_inactive_time(NULL) < 1000)
        {
           
            xQueueSend(IdleBreak_MessageQueue, &IdleBreakstr, 0);
        }

        /* LVGL 主循环：处理输入、渲染、动画 */
        lv_task_handler();

        {
            uint8_t f = CST816_Get_FingerNum();

            if (f != last_f)
            {
                last_f = f;
                edge_cnt++;
                dbg_puts("[TOUCH] finger=");
                dbg_putnum(f);
                if (f != 0x00 && f != 0xFF)
                {
                    CST816_Get_XY_AXIS();
                    dbg_puts("  X=");
                    dbg_putnum(CST816_Instance.X_Pos);
                    dbg_puts("  Y=");
                    dbg_putnum(CST816_Instance.Y_Pos);
                    dbg_puts("  (screen ");
                    dbg_putnum(LCD_W);
                    dbg_puts("x");
                    dbg_putnum(LCD_H);
                    dbg_puts(")");
                }
                dbg_puts("");
            }
            else if (++tick >= 3000)
            {
                tick = 0;
                dbg_puts("[TOUCH] heartbeat finger=");
                dbg_putnum(f);
                dbg_puts("  edges=");
                dbg_putnum((int32_t)edge_cnt);
                dbg_puts("  freeheap=");
                dbg_putnum((int32_t)xPortGetFreeHeapSize());
                dbg_puts("  freestack=");
                dbg_putnum((int32_t)uxTaskGetStackHighWaterMark(NULL));
            }
        }

        /* 延时 1ms，让出 CPU */
        vTaskDelay(pdMS_TO_TICKS(1));
    }
}
