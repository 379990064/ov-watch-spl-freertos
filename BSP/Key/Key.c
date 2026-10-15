#include "stm32f4xx.h"                  // Device header
#include "Delay.h"
#include "FreeRTOS.h"
#define KEY1        GPIO_ReadPinBit(GPIOA, GPIO_Pin_0)
/**
  * 函    数：按键初始化
  * 参    数：无
  * 返 回 值：无
  */
void Key_Init(void)
{
	/*开启时钟*/
	RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOA, ENABLE);		//开启GPIOB的时钟
	
	/*GPIO初始化*/
	GPIO_InitTypeDef GPIO_InitStructure;
	GPIO_InitStructure.GPIO_Mode = GPIO_Mode_IPU;
	GPIO_InitStructure.GPIO_Pin = GPIO_Pin_0;
	GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
	GPIO_Init(GPIOA, &GPIO_InitStructure);						//将PB1和PB11引脚初始化为上拉输入
}

/**
  * 函    数：按键获取键码
  * 参    数：无
  * 返 回 值：按下按键的键码值，范围：0~2，返回0代表没有按键按下
  * 注意事项：此函数是阻塞式操作，当按键按住不放时，函数会卡住，直到按键松手
  */
uint8_t Key_GetNum(uint8_t mode)
{
	uint8_t Keyvalue = 0;		//定义变量，默认键码值为0
	static uint8_t KeyNum = 1;

	if(mode) KeyNum=1;
	if(KeyNum &&KEY1)
	{
		vTaskDelay(3);
		if(!KEY1) 
		{
			Keyvalue =1;
			KeyNum=0;
		}
	}
	else
	{
		//delay_ms(3);//ensure the key is up
		vTaskDelay(3);
		if(KEY1)
			KeyNum = 1;
	}
	return Keyvalue;

}
