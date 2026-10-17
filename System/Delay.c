#include "stm32f4xx.h"                  // Device header
#include "Delay.h"

/* ============================================================================
 * 延时实现 —— ★ 绝不能使用 SysTick！
 *
 * 原来的实现是：
 *     SysTick->LOAD = reload * xus - 1U;
 *     SysTick->VAL  = 0x00;
 *     SysTick->CTRL = 0x00000005;      // 清掉 TICKINT
 *     while(!(SysTick->CTRL & 0x00010000));
 *     SysTick->CTRL = 0x00000004;      // 连 ENABLE 也清掉
 *
 * 这在调度器启动之前没问题，但 SysTick 在 vTaskStartScheduler() 之后
 * 是 FreeRTOS 的 1kHz 系统节拍源。上面最后一条把 ENABLE 清零，
 * 等于【永久关闭系统节拍】，之后所有 vTaskDelay / 超时 / 软件定时器全部停摆。
 *
 * 触发路径（每次触摸都会走到）：
 *     lv_task_handler() -> lv_port_indev -> CST816_Get_XY_AXIS
 *     -> IIC_Read_Multi_Byte -> myi2c_Sendbyte -> Delay_us
 * 所以第一次读触摸就把整个 FreeRTOS 时间基准杀死了。
 * 另外 CST816_RESET() 里的 Delay_ms 也会从任务上下文被调用
 * （user_RunModeTasks.c 的唤醒流程）。
 *
 * 原版为什么会没事：原版 SYSTEM\delay.c 在 OS_SUPPORT 0 时的实现只是
 * 【轮询 SysTick->VAL】，从头到尾不写 CTRL，所以不会破坏节拍。
 *
 * 现在改成用 Cortex-M4 的 DWT 周期计数器，完全不碰 SysTick，
 * 因此在调度器之前、之后、以及中断里都可以安全调用。
 * ========================================================================== */

/* ---- DWT 周期计数器支持 ---- */
#define DWT_CONTROL     (*(volatile uint32_t *)0xE0001000)
#define DWT_CYCCNT      (*(volatile uint32_t *)0xE0001004)
#define DEMCR           (*(volatile uint32_t *)0xE000EDFC)
#define DEMCR_TRCENA    (1UL << 24)
#define DWT_CTRL_CYCCNTENA (1UL << 0)

/* DWT 是否可用（有些情况下内核不允许使能 CYCCNT，则回退到软件循环） */
static uint8_t s_dwt_ok = 0;

/* 防止 -O2 把空循环优化掉 */
static volatile uint32_t s_dummy;

/**
  * @brief  初始化延时基准（DWT 周期计数器）
  *         在 main() 最早处、SystemClock_Config() 之后调用一次即可。
  *         即使不调用，Delay_us() 也会在第一次使用时自己初始化并回退。
  * @param  无
  * @retval 无
  */
void Delay_Init(void)
{
	DEMCR |= DEMCR_TRCENA;          /* 使能跟踪与调试模块（DWT 的前提） */
	DWT_CYCCNT = 0;                 /* 清周期计数 */
	DWT_CONTROL |= DWT_CTRL_CYCCNTENA;   /* 启动周期计数器 */

	/* 验证真的在走：连续读两次，值应当变化 */
	{
		uint32_t a = DWT_CYCCNT;
		uint32_t b = DWT_CYCCNT;
		s_dwt_ok = (a != b) ? 1U : 0U;
	}
}

/**
  * @brief  微秒级延时
  *         ★ 不接触 SysTick，因此在 FreeRTOS 任务/中断中都可安全调用。
  * @param  xus 延时时长（微秒）
  * @retval 无
  */
void Delay_us(uint32_t xus)
{
	uint32_t start;
	uint32_t ticks;

	if (xus == 0U) return;

	if (s_dwt_ok == 0U)
	{
		Delay_Init();               /* 首次调用时自动初始化 */
	}

	if (s_dwt_ok)
	{
		/* DWT_CYCCNT 每个 HCLK 周期加 1，所以 1us = SystemCoreClock/1000000 个计数 */
		ticks = xus * (SystemCoreClock / 1000000U);
		start = DWT_CYCCNT;

		/* 用无符号减法天然处理 32 位回绕 */
		while ((DWT_CYCCNT - start) < ticks)
		{
			/* 空转，纯计数等待 */
		}
	}
	else
	{
		/* 回退方案：粗略软件循环（不精确，但绝不碰 SysTick）。
		 * 约 4 个周期一次迭代，按 100MHz / 4 = 25 次每微秒估算。 */
		ticks = xus * (SystemCoreClock / 4000000U);
		while (ticks--)
		{
			s_dummy++;
		}
	}
}

/**
  * @brief  毫秒级延时
  *         ★ 现在也基于 DWT，不碰 SysTick，任务里调用不会再破坏系统节拍。
  *           不过在 FreeRTOS 任务里仍然推荐用 vTaskDelay()，
  *           因为 vTaskDelay 会让出 CPU，而这个函数是忙等。
  * @param  xms 延时时长（毫秒）
  * @retval 无
  */
void Delay_ms(uint32_t xms)
{
	while (xms--)
	{
		Delay_us(1000);
	}
}

/**
  * @brief  秒级延时
  * @param  xs 延时时长（秒）
  * @retval 无
  */
void Delay_s(uint32_t xs)
{
	while (xs--)
	{
		Delay_ms(1000);
	}
}
