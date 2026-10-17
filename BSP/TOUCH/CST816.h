#ifndef __CST816_H
#define __CST816_H

#include "stdint.h"
#include "myi2c.h"
#include "delay.h"
#include "stm32f4xx.h"                  // Device header


/* CST816 dev pin defination */

/* ★ 修正（原来写的是 PA15！）
 *   原定义 TOUCH_RST_PORT=GPIOA / TOUCH_RST_PIN=GPIO_Pin_15 与 LCD 背光
 *   （lcd_init.h 的 BLK_PIN=PA15、tim.c 的 TIM2_CH1=PA15）是同一个引脚。
 *   结果 CST816_RESET() 会把 PA15 从 TIM2 复用输出改成普通 GPIO 并恒定拉高，
 *   导致：① 背光 PWM 彻底失效、恒定全亮；② 触摸复位线被钉死、CST816 收不到
 *   有效复位时序，触摸完全无响应。
 *   实际硬件：触摸复位 = PB8，背光 = PA15。 */
#define TOUCH_RST_PIN  GPIO_Pin_8
#define TOUCH_INT_PIN  GPIO_Pin_1
#define TOUCH_RST_PORT GPIOB
#define TOUCH_INT_PORT GPIOB

/* functions define */
#define TOUCH_RST_0 GPIO_WriteBit(TOUCH_RST_PORT, TOUCH_RST_PIN, RESET)
#define TOUCH_RST_1 GPIO_WriteBit(TOUCH_RST_PORT, TOUCH_RST_PIN, SET)

/* 设备地址 */
#define Device_Addr 	0x15

/* 触摸屏寄存器 */
#define GestureID 			0x01
#define FingerNum 			0x02
#define XposH 					0x03
#define XposL 					0x04
#define YposH 					0x05
#define YposL 					0x06
#define ChipID 					0xA7
#define SleepMode				0xE5
#define MotionMask 			0xEC
#define IrqPluseWidth 	0xED
#define NorScanPer 			0xEE
#define MotionSlAngle 	0xEF
#define LpAutoWakeTime 	0xF4
#define LpScanTH 				0xF5
#define LpScanWin 			0xF6
#define LpScanFreq 			0xF7
#define LpScanIdac 			0xF8
#define AutoSleepTime 	0xF9
#define IrqCtl 					0xFA
#define AutoReset 			0xFB
#define LongPressTime 	0xFC
#define IOCtl 					0xFD
#define DisAutoSleep 		0xFE

/* 触摸屏坐标结构体 */
typedef struct
{
	unsigned int X_Pos;
	unsigned int Y_Pos;
} CST816_Info;

/* 全局实例（定义在 CST816.c），用于取回最近一次读取的触摸坐标 */
extern CST816_Info CST816_Instance;

/* 手势ID识别选项 */
typedef enum
{
	NOGESTURE = 	0x00,
	DOWNGLIDE = 	0x01,
	UPGLIDE = 		0x02,
	LEFTGLIDE = 	0x03,
	RIGHTGLIDE = 	0x04,
	CLICK = 			0x05,
	DOUBLECLICK = 0x0B,
	LONGPRESS = 	0x0C,
} GestureID_TypeDef;

/* 连续动作配置选项 */
typedef enum
{
	M_DISABLE = 	0x00,
	EnConLR = 		0x01,
	EnConUD = 		0x02,
	EnDClick = 		0x03,
	M_ALLENABLE = 0x07,
} MotionMask_TypeDef;

/* 中断低脉冲发射方式选项 */
typedef enum
{
	OnceWLP = 		0x00,
	EnMotion = 		0x10,
	EnChange = 		0x20,
	EnTouch = 		0x40,
	EnTest = 			0x80,
} IrqCtl_TypeDef;

/* 触摸屏初始化相关函数 */
void CST816_GPIO_Init(void);
void CST816_RESET(void);
void CST816_Init(void);

/* 触摸屏操作函数 */
void CST816_Get_XY_AXIS(void);
uint8_t CST816_Get_ChipID(void);
uint8_t CST816_Get_FingerNum(void);
/* 触摸屏读写函数 */
void CST816_IIC_WriteREG(uint8_t addr, uint8_t dat);
uint8_t CST816_IIC_ReadREG(unsigned char addr);

/* 触摸屏有关参数配置函数 */
void CST816_Config_MotionMask(uint8_t mode);
void CST816_Config_AutoSleepTime(uint8_t time);
void CST816_Config_MotionSlAngle(uint8_t x_right_y_up_angle);
void CST816_Config_NorScanPer(uint8_t Period);
void CST816_Config_IrqPluseWidth(uint8_t Width);
void CST816_Config_LpScanTH(uint8_t TH);
void CST816_Wakeup(void);
void CST816_Sleep(void);

#endif
