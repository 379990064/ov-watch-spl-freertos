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

/* ★ 空闲计时改用系统节拍累加，不再用软件定时器（省堆，见下面说明）。
 *   原版是 osTimerNew(..., osTimerPeriodic) + osTimerStart(...,100)。
 *   本工程 configUSE_TIMERS 关闭，改在 TaskTickHook() 里按 1kHz 计数，
 *   每满 100 个 tick（=100ms）给 IdleTimerCount 加 100 毫秒。 */
static uint16_t s_idle_tick_div = 0;

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
    /* 创建消息队列
     * ★ 容量按原版来（原版用的是 osMessageQueueNew(1, 1, NULL)）：
     *   Key/Idle/Stop/IdleBreak/HomeUpdata 容量 1，DataSave 容量 2。
     *   原来移植版写成 10，比原版多占 6*9=54 字节内存，
     *   在本工程 RAM 已经很紧（110KB/128KB）的情况下没必要。 */
    Key_MessageQueue      = xQueueCreate(1, 1);
    Idle_MessageQueue     = xQueueCreate(1, 1);
    Stop_MessageQueue     = xQueueCreate(1, 1);
    IdleBreak_MessageQueue= xQueueCreate(1, 1);
    HomeUpdata_MessageQueue = xQueueCreate(1, 1);
    DataSave_MessageQueue = xQueueCreate(2, 1);

    /* 创建硬件事件组（UART 事件由中断置位） */
    HardIntEventHandle = xEventGroupCreate();

    /* ★ 按键 GPIO 初始化（KEY1 = PA0，上拉输入）。
     *   原来整个工程从来没有调用过 Key_Init()，链接器把它当无用代码丢掉了
     *   （map 里可见 "Removing key.o(i.Key_Init)"），PA0 一直保持复位默认状态。
     *   必须在 KeyTask 开始扫描之前配置好。 */
    Key_Init();

/* ============================================================================
 * ★★★ 任务栈大小 —— 这里原来是致命的 4 倍膨胀 ★★★
 *
 *   原版用的是 CMSIS-RTOS v2 的 osThreadNew，而 osThreadAttr_t.stack_size
 *   的单位是【字节】，包装层会自己除以 4：
 *       // cmsis_os2.c:472-476
 *       stack = attr->stack_size / sizeof(StackType_t);
 *   所以原版的 128*10 字节 = 320 个字。
 *
 *   而原生 xTaskCreate 的第 3 个参数单位是【字】。移植时把同一个字面量
 *   128*10 直接搬了过来，实际占用变成 1280 字 = 5120 字节，【整整 4 倍】。
 *
 *   7 个任务的实际栈占用因此从本应 8448 字节涨到 33792 字节，
 *   而 configTOTAL_HEAP_SIZE 只有 30720 字节 —— 光栈就超了，
 *   还没算 7 个 TCB 和 heap_4 的块头。
 *
 *   后果：排在后面的 ScrRenewTask 创建失败（xTaskCreate 返回失败、
 *   句柄保持 NULL，因为没有定义 configASSERT，失败是【完全静默】的）。
 *   而 ScrRenewTask 正是消费 Key_MessageQueue -> Page_Back() 的任务，
 *   所以表现为「屏幕有画面，但点击和按键都没反应」。
 *
 *   下表按原版的实际字节数换算成字数（÷4）：
 *       任务               原版字节    正确字数
 *       HardwareInitTask    1280   ->   320
 *       LvHandlerTask       3072   ->   768
 *       IdleEnterTask        128   ->    32
 *       StopEnterTask       2048   ->   512
 *       KeyTask              128   ->    32
 *       ScrRenewTask        1280   ->   320
 *       MessageSendTask      512   ->   128
 *                          -----       ----
 *       合计                8448       2112 字 = 8448 字节（与原版一致）
 * ========================================================================== */

xTaskCreate(
    HardwareInitTask,          // 任务函数
    "HardwareInitTask",        // 任务名
    128 * 10 / 4,              // 320 字 = 1280 字节（原版值）
    NULL,                      // 传给任务的参数
    /* ★ 原来是 configMAX_PRIORITIES（=20），但 FreeRTOS 的合法优先级是
     *   0 .. configMAX_PRIORITIES-1，也就是 0..19！
     *   FreeRTOS 会把越界值静默钳到 19，但依赖钳位不是好习惯。
     *   改成 configMAX_PRIORITIES-1（=19），仍然是最高优先级，
     *   语义与原版 osPriorityHigh3（最高）一致：先把初始化跑完。 */
    configMAX_PRIORITIES - 1,
    &HardwareInitTaskHandle    // 任务句柄
);

xTaskCreate(
    LvHandlerTask,
    "LvHandlerTask",
    128 * 24 / 4,              // 768 字 = 3072 字节（原版值）
    NULL,
    8,
    &LvHandlerTaskHandle
);

xTaskCreate(
    IdleEnterTask,
    "IdleEnterTask",
    128 * 1 / 4,               // 32 字 = 128 字节（原版值）
    NULL,
    16,
    &IdleEnterTaskHandle
);

xTaskCreate(
    StopEnterTask,
    "StopEnterTask",
    128 * 16 / 4,              // 512 字 = 2048 字节（原版值）
    NULL,
    17,
    &StopEnterTaskHandle
);

xTaskCreate(
    KeyTask,
    "KeyTask",
    128 * 1 / 4,               // 32 字 = 128 字节（原版值）
    NULL,
    13,
    &KeyTaskHandle
);

xTaskCreate(
    ScrRenewTask,
    "ScrRenewTask",
    128 * 10 / 4,              // 320 字 = 1280 字节（原版值）
    NULL,
    10,
    &ScrRenewTaskHandle
);

xTaskCreate(
    MessageSendTask,
    "MessageSendTask",
    128 * 4 / 4,               // 128 字 = 512 字节（原版值）
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



    /* ★ 空闲计时（替代原版的 100ms 软件定时器）
     *   原版 IdleTimerCallback 每 100ms 给 IdleTimerCount 加 1，判断式是
     *   IdleTimerCount == ui_LTimeValue*10 （单位 100ms）。
     *   本移植版的 IdleEnterTask 判断式是
     *   IdleTimerCount >= ui_LTimeValue*1000 （单位 毫秒），
     *   所以这里按 1kHz 节拍每满 100 个 tick 就加 100 毫秒，量纲正好对上。 */
    if (++s_idle_tick_div >= 100)
    {
        s_idle_tick_div = 0;
        IdleTimerCount += 100;          /* 单位：毫秒 */
    }

    /* ★ 补回原版 TaskTickHook 里漏掉的一行（原版 user_TasksInit.c:238）
     *   user_HR_timecount 是心率算法的采样时基，
     *   user_SensUpdateTask.c:98 把它传给 HR_Calculate()。 */
    user_HR_timecount += 1;

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
    uint32_t tick = 0;
    uint8_t  last_f = 0xFF;
    uint32_t edge_cnt = 0;

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

        /* ===== 临时诊断：直接看触摸芯片报了什么 =====
         * 每次「按下 / 松开」状态变化都立刻打印一行，另外每约 3 秒打一次心跳。
         * 用来区分「任务没在跑」「芯片没报触摸」「坐标方向不对」。
         * 只做读操作，不干预 LVGL。排查完请删掉。
         *   edges = 累计检测到的状态变化次数，用来确认真的在读；
         *   如果手指按住不动会只打印一次 finger=1，松开时再打印一次 finger=0。 */
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
                    /* 顺便报告 LVGL 认为的屏幕尺寸，确认坐标有没有越界 */
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

        /* 延时 1ms，让出 CPU
         * ★ 任务里必须用 vTaskDelay，不能用 Delay_ms —— Delay_ms 内部是
         *   SysTick 忙等，会抢走 FreeRTOS 的系统节拍。 */
        vTaskDelay(pdMS_TO_TICKS(1));
    }
}
