#ifndef __USER_TASKSINIT_H__
#define __USER_TASKSINIT_H__

#ifdef __cplusplus
extern "C" {
#endif

/* ---------- FreeRTOS 原生头文件 ---------- */
#include "FreeRTOS.h"       // 必须最先
#include "task.h"           // 任务
#include "queue.h"          // 消息队列
#include "event_groups.h"   // 事件组（对应 osEventFlags）

/* ---------- 用户宏 ---------- */
#define SCRRENEW_DEPTH      5
#define HARDINT_EVENT_UART  (1U << 0)
#define HARDINT_EVENT_CHARG (1U << 1)
#define HARDINT_EVENT_MPU   (1U << 2)

/* ---------- 全局句柄声明 ---------- */
extern QueueHandle_t      Key_MessageQueue;
extern QueueHandle_t      Idle_MessageQueue;
extern QueueHandle_t      Stop_MessageQueue;
extern QueueHandle_t      IdleBreak_MessageQueue;
extern QueueHandle_t      HomeUpdata_MessageQueue;
extern QueueHandle_t      DataSave_MessageQueue;
extern EventGroupHandle_t HardIntEventHandle;

/* ---------- 函数声明 ---------- */
/* ★ 原来这里声明的是 User_Tasks_Init()，但 user_TasksInit.c 里实现的函数叫
 *   task_init()，两边对不上 —— main.c 调用 task_init() 时只能靠隐式声明，
 *   编译器会当成 int task_init() 处理，属于隐患。这里改成真实的名字。 */
void task_init(void);
void TaskTickHook(void);
void LvHandlerTask(void *argument);

#ifdef __cplusplus
}
#endif

#endif /* __USER_TASKSINIT_H__ */
