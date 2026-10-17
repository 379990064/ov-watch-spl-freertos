/* main.c —— 主程序（标准库 SPL + FreeRTOS + LVGL）
   调试输出：直接写 USART1 寄存器（绕过 printf/重定向，MicroLIB 未开也能用） */

/* Includes ------------------------------------------------------------------*/
#include "main.h"
#include "stm32f4xx.h"                  // Device header
#include "stm32f4xx_rcc.h"               // RCC_GetFlagStatus / RCC_FLAG_*RST（复位原因诊断）
#include "MYSPI.h"                      // SPI1（LCD）
#include "tim.h"                        // 背光 TIM2（PA15）
#include "Serial.h"                     // USART1
#include "lcd.h"                        // LCD_Init / LCD_Fill
#include "lcd_init.h"                   // LCD_W / LCD_H
#include "delay.h"                      // Delay_ms
#include "rtc.h"                        // MX_RTC_Init / RTC_MspInit
#include "CST816.h"                     // CST816_GPIO_Init / CST816_RESET / CST816_Get_ChipID
#include "version.h"                    // watch_version_major/minor/patch
#include <stdio.h>                      // sprintf
#include "lvgl.h"                       // lv_init
#include "lv_port_disp.h"               // lv_port_disp_init
#include "lv_port_indev.h"              // lv_port_indev_init
#include "ui.h"                         // ui_init
#include "user_TasksInit.h"             // task_init()
#include "FreeRTOS.h"
#include "task.h"

/* 直接写 USART1 的调试输出（不依赖 printf/重定向）
 * ★ 改成非 static 并对外声明（见 Serial.h），因为本工程用的是 MicroLIB
 *   且没有做 _sys_exit/_ttywrch/__use_no_semihosting 重定向，
 *   printf 在缺这些符号时会走到半主机 BKPT 上把程序卡死。
 *   所以全工程统一用这个直写寄存器的版本来打调试信息。 */
void dbg_send(uint8_t c)
{
    while (USART_GetFlagStatus(USART1, USART_FLAG_TXE) == RESET);
    USART_SendData(USART1, c);
}
void dbg_puts(const char *s)
{
    while (*s) dbg_send((uint8_t)*s++);
    dbg_send('\r');
    dbg_send('\n');
}

/* 十进制打印（避免为了打印一个数字而引入 printf） */
void dbg_putnum(int32_t v)
{
    char buf[12];
    int  i = 0;

    if (v < 0) { dbg_send('-'); v = -v; }
    if (v == 0) { dbg_send('0'); return; }

    while (v > 0 && i < (int)sizeof(buf)) { buf[i++] = (char)('0' + (v % 10)); v /= 10; }
    while (i > 0) dbg_send((uint8_t)buf[--i]);
}

/* Private function prototypes -----------------------------------------------*/
void SystemClock_Config(void);

/**
 * @brief  The application entry point.
 * @retval int
 */
int main(void)
{
    /* ★ 向量表基址：对齐 Keil 的 IROM 设置。
     *   本工程 IROM 从 0x08000000 起烧，而 system_stm32f4xx.c 里
     *   VECT_TAB_OFFSET = 0x00，所以 SystemInit() 已经设成 0x08000000 了。
     *   这里显式写一次，是为了以后加 bootloader / 改 IROM 偏移时不会踩坑。
     *   （原 HAL 工程写的是 0x00000000，那是指向别名地址 0x00000000，
     *     和 0x08000000 指向同一块 Flash，效果等价。） */
    SCB->VTOR = 0x08000000U;

    /* ★ 必须最先设置 NVIC 优先级分组！
     *   原版是 HAL 工程，HAL_Init() 内部会自动调用
     *       HAL_NVIC_SetPriorityGrouping(NVIC_PRIORITYGROUP_4)
     *   而移植版用的是标准库，【没有任何地方】设置过分组。
     *
     *   后果很严重：SPL 的 NVIC_Init() 是从 SCB->AIRCR 的 PRIGROUP 字段
     *   反推抢占位数的（见 FWLIB\src\misc.c）。复位后 PRIGROUP=0，
     *   算出来的移位量溢出（未定义行为），结果【所有中断优先级都被写成 0】，
     *   也就是全部变成最高优先级。于是：
     *     - 它们不会被 FreeRTOS 的 taskENTER_CRITICAL 屏蔽；
     *     - 中断里调用 FreeRTOS 的 FromISR 接口会破坏内核状态。
     *
     *   NVIC_PriorityGroup_4 = 4 位全部用作抢占优先级，不分子优先级，
     *   正是原版 HAL_Init() 使用的分组，也是 FreeRTOS 官方推荐的分组。 */
    NVIC_PriorityGroupConfig(NVIC_PriorityGroup_4);

    /* 0. 最先初始化串口（用当前默认时钟），确认 MCU 已运行 */
    MX_USART1_UART_Init();
    dbg_puts("[BOOT] mcu alive");

    /* ★ 复位原因诊断（必须在串口初始化之后打印）
     *    RCC_CSR 的复位标志会一直保持到手动清除，所以能反映「这次是怎么起来的」：
     *      PINRST  复位引脚（外部复位 / 有些外部看门狗也接这里）
     *      POWRST  上电或掉电复位 -> 供电问题（电池没电 / LDO 带不动 / 接触不良）
     *      SFTRST  软件复位       -> 代码里调了 NVIC_SystemReset()
     *      IWDGRST 独立看门狗     -> 本工程没启用 IWDG，出现即异常
     *      WWDGRST 窗口看门狗     -> 同上
     *      LPWRRST 低功耗复位     -> STOP/STANDBY 相关问题
     *    正常上电/按键复位会是 POWRST + PINRST，属正常。 */
    {
        dbg_puts("[RST ] cause:");
        if (RCC_GetFlagStatus(RCC_FLAG_BORRST)  != RESET) dbg_puts("    BORRST  (brown-out!) -> power supply");
        if (RCC_GetFlagStatus(RCC_FLAG_PINRST)  != RESET) dbg_puts("    PINRST  (reset pin)");
        if (RCC_GetFlagStatus(RCC_FLAG_PORRST)  != RESET) dbg_puts("    PORRST  (power-on)");
        if (RCC_GetFlagStatus(RCC_FLAG_SFTRST)  != RESET) dbg_puts("    SFTRST  (software reset)");
        if (RCC_GetFlagStatus(RCC_FLAG_IWDGRST) != RESET) dbg_puts("    IWDGRST (independent watchdog)");
        if (RCC_GetFlagStatus(RCC_FLAG_WWDGRST) != RESET) dbg_puts("    WWDGRST (window watchdog)");
        if (RCC_GetFlagStatus(RCC_FLAG_LPWRRST) != RESET) dbg_puts("    LPWRRST (low-power mode)");

        RCC_ClearFlag();        /* 清掉，下次重启才能反映新的原因 */
    }

    /* 1. 系统时钟配置（HSI + PLL → SYSCLK=100MHz，不依赖外部晶振） */
    SystemClock_Config();
    /* 时钟变了，重配串口波特率 */
    MX_USART1_UART_Init();
    dbg_puts("[BOOT] clock ok");

    /* 2. ★ 释放 JTAG（PA15/PB3/PB4 → 普通 IO）
     *    PB3 = SPI1_SCK，PB4 = LCD_RES，PA15 = 背光；
     *    F4 复位后 SWJ 默认是 JTAG-DP + SW-DP，会占住这三根脚，
     *    不释放的话 SPI 根本发不出时钟，屏幕必然不亮。
     *    SWJ_CFG = 0b010 → 关闭 JTAG、保留 SWD，可继续下载调试。 */
    RCC_APB2PeriphClockCmd(RCC_APB2Periph_SYSCFG, ENABLE);
    SYSCFG->MEMRMP = (SYSCFG->MEMRMP & 0xFC00FFFF) | 0x02000000;
    dbg_puts("[BOOT] jtag release ok");

    /* 3. 外设初始化 */
    MX_SPI1_Init();
    dbg_puts("[BOOT] spi1 ok");

    /* 背光：BLK=PA15=TIM2_CH1，必须先初始化并给出占空比，
     * 否则 PA15 复位后是浮空输入，背光不亮，屏上什么也看不见 */
    MX_TIM2_Init();
    TIM2_SetPulse(150);                 // 周期 300 → 50% 占空比点亮背光
    dbg_puts("[BOOT] tim2 backlight ok");

    /* 3.5 确保中断使能。
     *     SPL 复位后 PRIMASK 本来就是 0（中断已开），这里显式写一次，
     *     对齐原 HAL 工程 SysInit 里那句 __enable_irq()。 */
    __enable_irq();

    /* 3.6 RTC 初始化（对应原工程 MX_RTC_Init）。
     *     必须在调度器启动之前做完：它内部要用 RTC_WaitForSynchro()
     *     这类同步等待，放进任务里会因为抢占出问题。
     *     注意：HAL 的 HAL_RTC_Init() 内部会调 RTC_MspInit() 去配 NVIC，
     *     而 SPL 的 MX_RTC_Init() 不会，所以这里要手动补一次。 */
    MX_RTC_Init();
    RTC_MspInit();
    dbg_puts("[BOOT] rtc ok");

    /* ==========================================================================
     * 4. ★ 屏幕 / 触摸 / LVGL 初始化
     *
     *    必须在 vTaskStartScheduler() 之前做完！原因：
     *      LCD_Init() 内部有 Delay_ms(100+100+120)，而 Delay_us() 是用
     *      SysTick 做忙等（SysTick->CTRL = 0x05 ... 最后 = 0x04 直接关掉），
     *      它会抢先占用并重配 FreeRTOS 用来做系统节拍的 SysTick。
     *      在调度器启动之前 SysTick 还是自由的，所以这里调用完全安全；
     *      一旦进了任务上下文再调，就会破坏节拍、把整个系统卡死
     *      （之前的现象就是卡在 HardwareInitTask 里的 LCD_Init 上）。
     * ========================================================================== */

    /* 4.1 屏幕 */
    LCD_Init();
    LCD_Fill(0, 0, LCD_W, LCD_H, BLACK);
    Delay_ms(10);
    dbg_puts("[BOOT] lcd init ok");

    /* 4.2 开机画面（Welcome + 版本号），保留 1 秒 */
    LCD_Set_Light(50);
    LCD_ShowString(72, LCD_H / 2, (uint8_t *)"Welcome!", WHITE, BLACK, 24, 0);
    {
        uint8_t lcd_buf_str[24];        /* ★ 原来是 17，版本号变长会溢出，改大 */
        sprintf((char *)lcd_buf_str, "OV-Watch V%d.%d.%d",
                watch_version_major(), watch_version_minor(), watch_version_patch());
        LCD_ShowString(34, LCD_H / 2 + 48, lcd_buf_str, WHITE, BLACK, 24, 0);
    }
    Delay_ms(1000);
    LCD_Fill(0, LCD_H / 2 - 24, LCD_W, LCD_H / 2 + 49, BLACK);
    dbg_puts("[BOOT] boot screen ok");

    /* 4.3 触摸 */
    CST816_GPIO_Init();
    CST816_RESET();
    {
        /* 只以 ChipID 为准判断 I2C 是否通。
         * ★ 不要再拿 SCL/SDA 的即时电平做判断 —— SCL 是开漏输出，
         *   上一笔事务结束时它停在低电平是正常的，据此报「总线被拉死」
         *   会误报（之前就误报过一次）。芯片能返回正确的 ChipID 就是
         *   总线正常的最可靠证据。 */
        uint8_t id = CST816_Get_ChipID();

        dbg_puts("[TOUCH] CST816 ChipID(0xA7)=");
        dbg_putnum(id);
        dbg_puts("   (expect 181 = 0xB5)");

        if (id == 0 || id == 0xFF)
            dbg_puts("[TOUCH] I2C NO ACK -> check SDA(PB7) / SCL(PB6) / RST(PB8) / power");
        else
            dbg_puts("[TOUCH] I2C OK");
    }

    /* 4.4 LVGL */
    lv_init();
    lv_port_disp_init();
    lv_port_indev_init();
    ui_init();
    dbg_puts("[BOOT] lvgl + ui init ok");

    /* 4.5 ★ 启动串口 DMA 接收 + IDLE 空闲中断
     *     对应原版 user_HardwareInitTask.c:58-59：
     *         HAL_UART_Receive_DMA(&huart1, HardInt_receive_str, 25);
     *         __HAL_UART_ENABLE_IT(&huart1, UART_IT_IDLE);
     *     移植版把这两句丢了，所以 MessageSendTask 永远收不到数据、
     *     一直阻塞在 portMAX_DELAY。处理函数见 stm32f4xx_it.c 的 USART1_IRQHandler。
     *     必须在 vTaskStartScheduler() 之前做，避免任务还没起来就先来中断。 */
    USART1_DMA_RX_Start((uint8_t *)HardInt_receive_str, 25);
    USART_ITConfig(USART1, USART_IT_IDLE, ENABLE);
    dbg_puts("[BOOT] uart dma rx armed");

    /* 5. 创建 FreeRTOS 任务 */
    task_init();
    dbg_puts("[BOOT] tasks created");

    /* 6. 启动调度器（控制权交给 FreeRTOS） */
    vTaskStartScheduler();

    /* 若走到这里，说明调度器启动失败（一般是堆不够） */
    dbg_puts("[BOOT] SCHEDULER FAILED!");
    while (1)
    {
    }
}

/**
 * @brief 系统时钟配置（对应参考 HAL 的 SystemClock_Config）
 *        HSI 16MHz → PLL(M=8, N=100, P=2) → SYSCLK=100MHz
 *        AHB=1, APB1=2, APB2=1, Flash 等待 3WS
 * @retval None
 */
void SystemClock_Config(void)
{
    /* 1. 使能 PWR 时钟，电压调节器 Scale1（100MHz 需要） */
    RCC_APB1PeriphClockCmd(RCC_APB1Periph_PWR, ENABLE);
    PWR->CR |= PWR_CR_VOS;              /* VOS=1 → Scale1 */

    /* 2. Flash 等待周期 3 + 使能缓存 */
    FLASH->ACR = FLASH_ACR_LATENCY_3WS | FLASH_ACR_ICEN | FLASH_ACR_DCEN | FLASH_ACR_PRFTEN;

    /* 3. 使能 HSI 并等待就绪 */
    RCC_HSICmd(ENABLE);
    while (RCC_GetFlagStatus(RCC_FLAG_HSIRDY) == RESET);

    /* 4. 配置 PLL：源=HSI，M=8，N=100，P=2，Q=4
     *    VCO 输入 = 16MHz / 8 = 2MHz，VCO = 2MHz * 100 = 200MHz
     *    SYSCLK = 200MHz / 2 = 100MHz，USB/SDIO 48MHz = 200/4（Q=4）
     *    注意：改用别的频率时，FreeRTOSConfig.h 的 configCPU_CLOCK_HZ
     *          必须跟着改，否则 FreeRTOS 节拍和 Delay 全都不准。 */
    RCC_PLLConfig(RCC_PLLSource_HSI, 8, 100, 2, 4);
    RCC_PLLCmd(ENABLE);
    while (RCC_GetFlagStatus(RCC_FLAG_PLLRDY) == RESET);

    /* 5. 总线分频：AHB=SYSCLK/1，APB1=SYSCLK/2，APB2=SYSCLK/1 */
    RCC_HCLKConfig(RCC_SYSCLK_Div1);
    RCC_PCLK1Config(RCC_HCLK_Div2);
    RCC_PCLK2Config(RCC_HCLK_Div1);

    /* 6. 系统时钟源 = PLL（RCC_GetSYSCLKSource 返回 0x08 表示 PLL） */
    RCC_SYSCLKConfig(RCC_SYSCLKSource_PLLCLK);
    while (RCC_GetSYSCLKSource() != 0x08);   /* 0x08：PLL 作为系统时钟 */

    /* 7. 更新 SystemCoreClock（Delay.c 按它算延时，必须正确） */
    SystemCoreClockUpdate();

    /* 8. ★ 初始化 DWT 延时基准（必须在 SystemCoreClock 更新之后）
     *    Delay_us/Delay_ms 现在基于 DWT 周期计数器，不再使用 SysTick，
     *    因此在 FreeRTOS 任务和中断里调用都不会破坏系统节拍。 */
    Delay_Init();

    /* 8. 这里不要自己调 SysTick_Config！
     *    FreeRTOS 的 xPortStartScheduler() 会自己设置 SysTick 作为系统节拍，
     *    提前配置只会被覆盖，还会在 vTaskStartScheduler() 之前产生多余的节拍中断。 */
}
