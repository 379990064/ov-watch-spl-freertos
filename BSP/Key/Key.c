#include "stm32f4xx.h"                  // Device header
#include "Delay.h"
#include "FreeRTOS.h"
#include "Key.h"
#include "task.h"

/**
  * 函    数：按键初始化
  * 参    数：无
  * 返 回 值：无
  */
void Key_Init(void)
{
	/*开启时钟*/
	RCC_AHB1PeriphClockCmd(RCC_AHB1Periph_GPIOA, ENABLE);		//开启GPIOB的时钟
	
	/*GPIO初始化*/
	GPIO_InitTypeDef GPIO_InitStructure;
	GPIO_InitStructure.GPIO_Mode = GPIO_Mode_IN;
	GPIO_InitStructure.GPIO_OType	=	GPIO_OType_PP;
	GPIO_InitStructure.GPIO_PuPd	=	GPIO_PuPd_UP;
	GPIO_InitStructure.GPIO_Pin = GPIO_Pin_0;
	GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
	GPIO_Init(GPIOA, &GPIO_InitStructure);						//将PB1和PB11引脚初始化为上拉输入
}


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

uint8_t KeyScan(uint8_t mode)
{
	return Key_GetNum(mode);
}
