/* Private includes -----------------------------------------------------------*/
// sys
#include "Serial.h"              // 标准库工程里串口叫 Serial.h
#include "tim.h"
#include "delay.h"
// user
#include "user_TasksInit.h"
#include "version.h"
// bsp
#include "lcd.h"
#include "lcd_init.h"
#include "CST816.h"
// ui / gui
#include "lvgl.h"
#include "lv_port_disp.h"
#include "lv_port_indev.h"
#include "ui.h"
// 其他
#include <stdio.h>               // sprintf

/**
 * @brief hardwares init task
 *        只初始化屏幕 + 触摸（外加背光与调试串口）
 *        传感器 / 蓝牙 / EEPROM / RTC 唤醒已按需求移除
 * @param argument: Not used
 * @retval None
 */
void HardwareInitTask(void *argument)
{
    while(1)
    {
        vTaskSuspendAll();

        // 背光：TIM2 PWM（PA15，周期 300），先上电再设占空比
        MX_TIM2_Init();
        TIM2_SetPulse(150);            // 50% 占空比
        TIM_Cmd(TIM2, ENABLE);

        // USART1 调试串口（printf / 其余任务用）
        MX_USART1_UART_Init();
        USART1_DMA_RX_Start((uint8_t*)HardInt_receive_str, 25);
        USART_ITConfig(USART1, USART_IT_IDLE, ENABLE);

        // 触摸
        CST816_GPIO_Init();
        CST816_RESET();

        // LCD 初始化 + 开机画面
        LCD_Init();
        LCD_Fill(0,0, LCD_W, LCD_H, BLACK);
        Delay_ms(10);
        LCD_Set_Light(50);
        LCD_ShowString(72,LCD_H/2,(uint8_t*)"Welcome!", WHITE, BLACK, 24, 0);
        uint8_t lcd_buf_str[17];
        sprintf(lcd_buf_str, "OV-Watch V%d.%d.%d", watch_version_major(), watch_version_minor(), watch_version_patch());
        LCD_ShowString(34, LCD_H/2+48, (uint8_t*)lcd_buf_str, WHITE, BLACK, 24, 0);
        Delay_ms(1000);
        LCD_Fill(0, LCD_H/2-24, LCD_W, LCD_H/2+49, BLACK);

        // LVGL
        lv_init();
        lv_port_disp_init();
        lv_port_indev_init();
        ui_init();

        xTaskResumeAll();
        vTaskDelete(NULL);
    }
}
