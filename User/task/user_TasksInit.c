#include "FreeRTOS.h"
#include "task.h"
#include "queue.h"
#include "timers.h"
#include "event_groups.h"
//sys
#include "sys.h"
#include "stdio.h"
#include "lcd.h"
//gui
#include "lvgl.h"
#include "ui_TimerPage.h"
//tasks
#include "user_TasksInit.h"
#include "user_HardwareInitTask.h"
#include "user_RunModeTasks.h"
#include "user_KeyTask.h"
#include "user_ScrRenewTask.h"
#include "user_MessageSendTask.h"
xTimerHandle IDLETimerHandle;

/* 任务句柄定义（供 xTaskCreate 与外部任务使用） */
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

/* 硬件事件组（UART 收到数据等） */
EventGroupHandle_t HardIntEventHandle;

void task_init(void)
{
    /* 创建消息队列（1 字节/项；容量可按原工程调整） */
    Key_MessageQueue      = xQueueCreate(10, 1);
    Idle_MessageQueue     = xQueueCreate(10, 1);
    Stop_MessageQueue     = xQueueCreate(10, 1);
    IdleBreak_MessageQueue= xQueueCreate(10, 1);
    HomeUpdata_MessageQueue = xQueueCreate(10, 1);
    DataSave_MessageQueue = xQueueCreate(10, 1);

    /* 创建硬件事件组（UART 事件由中断置位） */
    HardIntEventHandle = xEventGroupCreate();

xTaskCreate(
    HardwareInitTask,          // 任务函数
    "HardwareInitTask",        // 任务名
    128 * 10,                  // 栈深度（单位：StackType_t，通常是 4 字节 word）
    NULL,                      // 传给任务的参数
    configMAX_PRIORITIES    ,  // 优先级（数字）
    &HardwareInitTaskHandle    // 任务句柄
);

xTaskCreate(
    LvHandlerTask,
    "LvHandlerTask",
    128 * 24,
    NULL,
    8,
    &LvHandlerTaskHandle
);


xTaskCreate(
    IdleEnterTask,
    "IdleEnterTask",
    128 * 1,
    NULL,
    16,
    &IdleEnterTaskHandle
);



xTaskCreate(
    StopEnterTask,
    "StopEnterTask",
    128 * 16,
    NULL,
    17,
    &StopEnterTaskHandle
);

xTaskCreate(
    KeyTask,
    "KeyTask",
    128 * 1,
    NULL,
    13,
    &KeyTaskHandle
);

xTaskCreate(
    ScrRenewTask,
    "ScrRenewTask",
    128 * 10,
    NULL,
    10,
    &ScrRenewTaskHandle
);

xTaskCreate(
    MessageSendTask,
    "MessageSendTask",
    128 * 4,
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



    // ★ 如果唤醒了更高优先级任务，请求切换
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

    while (1)
    {
        /* 最近 1000ms 内有用户操作 → 重置空闲计时，保持亮屏 */
        if (lv_disp_get_inactive_time(NULL) < 1000)
        {
            /* 任务上下文，用 xQueueSend，超时 0（不阻塞） */
            xQueueSend(IdleBreak_MessageQueue, &IdleBreakstr, 0);
        }

        /* LVGL 主循环：处理输入、渲染、动画 */
        lv_task_handler();

        /* 延时 1ms，让出 CPU */
        vTaskDelay(pdMS_TO_TICKS(1));
    }
}
