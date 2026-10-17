/* myi2c.c —— 软件 I2C（CST816 触摸屏）*/

#include "stm32f4xx.h"                  // Device header
#include "Delay.h"
#include "myi2c.h"


#define SCL      GPIO_Pin_6
#define SDA      GPIO_Pin_7
#define i2c_Port GPIOB


static void sda_output_mode(void)
{
	GPIO_InitTypeDef GPIO_InitStructure;

	GPIO_InitStructure.GPIO_Pin   = SDA;
	GPIO_InitStructure.GPIO_Mode  = GPIO_Mode_OUT;
	GPIO_InitStructure.GPIO_OType = GPIO_OType_OD;   
	GPIO_InitStructure.GPIO_PuPd  = GPIO_PuPd_UP;
	GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
	GPIO_Init(i2c_Port, &GPIO_InitStructure);
}

static void sda_input_mode(void)
{
	GPIO_InitTypeDef GPIO_InitStructure;

	GPIO_InitStructure.GPIO_Pin   = SDA;
	GPIO_InitStructure.GPIO_Mode  = GPIO_Mode_IN;    
	GPIO_InitStructure.GPIO_PuPd  = GPIO_PuPd_UP;
	GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
	GPIO_Init(i2c_Port, &GPIO_InitStructure);
}

void myi2c_Init(void)
{
	GPIO_InitTypeDef GPIO_InitStructure;

	RCC_AHB1PeriphClockCmd(RCC_AHB1Periph_GPIOB, ENABLE);


	GPIO_InitStructure.GPIO_Mode  = GPIO_Mode_OUT;
	GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
	GPIO_InitStructure.GPIO_Pin   = SCL;
	GPIO_InitStructure.GPIO_OType = GPIO_OType_OD;
	GPIO_InitStructure.GPIO_PuPd  = GPIO_PuPd_UP;
	GPIO_Init(i2c_Port, &GPIO_InitStructure);


	GPIO_InitStructure.GPIO_Pin = SDA;
	GPIO_Init(i2c_Port, &GPIO_InitStructure);

	GPIO_SetBits(i2c_Port, SCL | SDA);
}

void myi2c_SCL_W(BitAction value)
{
	GPIO_WriteBit(i2c_Port, SCL, value);
}

void myi2c_SDA_W(BitAction value)
{
	GPIO_WriteBit(i2c_Port, SDA, value);
}

uint8_t myi2c_SDA_R(void)
{
	return (uint8_t)GPIO_ReadInputDataBit(i2c_Port, SDA);
}

void myi2c_start(void)
{
	sda_output_mode();
	myi2c_SDA_W(Bit_SET);
	myi2c_SCL_W(Bit_SET);
	myi2c_SDA_W(Bit_RESET);
	myi2c_SCL_W(Bit_RESET);
}

void myi2c_end(void)
{
	myi2c_SDA_W(Bit_RESET);
	myi2c_SCL_W(Bit_SET);
	myi2c_SDA_W(Bit_SET);
	myi2c_SCL_W(Bit_RESET);
}

void myi2c_Sendbyte(uint8_t byte)
{
	uint8_t i;

	sda_output_mode();
	for(i = 0; i < 8; i++)
	{
		myi2c_SDA_W((byte & (0x80 >> i)) ? Bit_SET : Bit_RESET);
		Delay_us(2);
		myi2c_SCL_W(Bit_SET);
		Delay_us(1);
		myi2c_SCL_W(Bit_RESET);
		Delay_us(1);
	}
}

uint8_t myi2c_Receivebyte(void)
{
	uint8_t i, byte = 0x00;

	sda_input_mode();          /*  读之前切成输入 */

	for(i = 0; i < 8; i++)
	{
		Delay_us(2);
		myi2c_SCL_W(Bit_SET);
		Delay_us(1);
		if(myi2c_SDA_R() == 1)
			byte |= (0x80 >> i);
		myi2c_SCL_W(Bit_RESET);
		Delay_us(1);
	}

	sda_output_mode();         /*  读完切回输出 */
	return byte;
}

void myi2c_Sendack(uint8_t ack)
{
	sda_output_mode();
	/* ack=1 → 发送 NACK（释放 SDA 为高）；ack=0 → 发送 ACK（拉低） */
	myi2c_SDA_W(ack ? Bit_SET : Bit_RESET);
	Delay_us(1);
	myi2c_SCL_W(Bit_SET);
	Delay_us(1);
	myi2c_SCL_W(Bit_RESET);
	Delay_us(1);
}

uint8_t myi2c_Receiveack(void)
{
	uint8_t errtime = 5;

	sda_input_mode();         
	Delay_us(1);
	myi2c_SCL_W(Bit_SET);
	Delay_us(1);

	while(myi2c_SDA_R())
	{
		errtime--;
		Delay_us(1);
		if(0 == errtime)
		{
			sda_output_mode();
			myi2c_end();
			return 1;          /* 1 = 无应答 */
		}
	}

	Delay_us(1);
	myi2c_SCL_W(Bit_RESET);
	sda_output_mode();
	return 0;                  /* 0 = 收到 ACK */
}

uint8_t IIC_Write_Multi_Byte(uint8_t daddr, uint8_t reg, uint8_t length, uint8_t buff[])
{
	unsigned char i;

	myi2c_start();
	myi2c_Sendbyte(daddr << 1);
	if(myi2c_Receiveack())
	{
		myi2c_end();
		return 1;
	}
	myi2c_Sendbyte(reg);
	myi2c_Receiveack();
	for(i = 0; i < length; i++)
	{
		myi2c_Sendbyte(buff[i]);
		myi2c_Receiveack();
	}
	myi2c_end();
	Delay_us(1);
	return 0;
}

uint8_t IIC_Read_Multi_Byte(uint8_t daddr, uint8_t reg, uint8_t length, uint8_t buff[])
{
	unsigned char i;

	myi2c_start();
	myi2c_Sendbyte(daddr << 1);
	if(myi2c_Receiveack())
	{
		myi2c_end();
		return 1;
	}
	myi2c_Sendbyte(reg);
	myi2c_Receiveack();

	myi2c_start();
	myi2c_Sendbyte((daddr << 1) + 1);
	myi2c_Receiveack();

	for(i = 0; i < length; i++)
	{
		buff[i] = myi2c_Receivebyte();
		if(i < length - 1)
		{
			myi2c_Sendack(0);      /* 继续读 → ACK */
		}
	}
	myi2c_Sendack(1);              /* 最后一个字节 → NACK */
	myi2c_end();
	return 0;
}
