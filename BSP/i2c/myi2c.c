#include "stm32f4xx.h"                  // Device header
#include "Delay.h"

#define SCL GPIO_Pin_6
#define SDA GPIO_Pin_4
#define i2c_Port GPIOB

void myi2c_Init(void)
{
 RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOB, ENABLE);
	
	GPIO_InitTypeDef GPIO_InitStructure;
 	GPIO_InitStructure.GPIO_Mode = GPIO_Mode_Out_OD;
	GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
	GPIO_InitStructure.GPIO_Pin = SCL | SDA;
 	GPIO_Init(i2c_Port, &GPIO_InitStructure);

	
	GPIO_SetBits(i2c_Port,	SCL	|	SDA);
	
}
	
void myi2c_SCL_W(BitAction value)
{
	GPIO_WriteBit(i2c_Port,	SCL,	value);
}

void myi2c_SDA_W(BitAction value)
{
	GPIO_WriteBit(i2c_Port,	SDA,	value);
}

uint8_t myi2c_SDA_R(void)
{
	uint8_t value;
	value	=	GPIO_ReadInputDataBit(i2c_Port,SDA);
	return value;
}

void myi2c_start(void)
{
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
	for(i=0;	i<8;	i++)
	{
		myi2c_SDA_W(byte & (0x80	>>	i));
		delay_us(2);
		myi2c_SCL_W(Bit_SET);
		delay_us(1);
		myi2c_SCL_W(Bit_RESET);
		delay_us(1);
	}
}
	
uint8_t myi2c_Receivebyte(void)
{
		uint8_t i,	byte	=	0x00;
	for(i=0;	i<8;	i++)
	{
		myi2c_SDA_W(Bit_SET);
		delay_us(2);
		myi2c_SCL_W(Bit_SET);
		delay_us(1);
		if(myi2c_SDA_R()	==	1)
			byte |= (0x80	>>	i);
		myi2c_SCL_W(Bit_RESET);
		delay_us(1);
	}
	return byte;
}

void myi2c_Sendack(uint8_t ack)
{

		myi2c_SDA_W(ack);
		delay_us(1);
		myi2c_SCL_W(Bit_SET);
		delay_us(1);
		myi2c_SCL_W(Bit_RESET);
		delay_us(1);

}
	
uint8_t myi2c_Receiveack(void)
{
	uint8_t errtime = 5;
	uint8_t i2c_ack	=	0;
	myi2c_SDA_W(Bit_SET);
	delay_us(1);
	myi2c_SCL_W(Bit_SET);
	delay_us(1);
	    while(myi2c_SDA_R())
    {
        errtime--;
		delay_us(1);
        if (0 == errtime)
        {
            myi2c_end();
            return 1;
        }
    }
			delay_us(1);
	myi2c_SCL_W(Bit_RESET);
	return 0;
}

uint8_t IIC_Write_Multi_Byte(uint8_t daddr,uint8_t reg,uint8_t length,uint8_t buff[])
{
	unsigned char i;
    myi2c_start();

	myi2c_Sendbyte(daddr<<1);
	if(myi2c_Receiveack())
	{
		myi2c_end();
		return 1;
	}
	myi2c_Sendbyte(reg);
	myi2c_Receiveack();
	for(i=0;i<length;i++)
	{
		Myi2c_Sendbyte(buff[i]);
		myi2c_Receiveack();
	}
    myi2c_end();
	delay_us(1);
	return 0;
}




uint8_t IIC_Read_Multi_Byte(uint8_t daddr, uint8_t reg, uint8_t length, uint8_t buff[])
{
	unsigned char i;
	myi2c_start();
	myi2c_Sendbyte(daddr<<1);
	if(myi2c_Receiveack())
	{
		myi2c_end();
		return 1;
	}
	myi2c_Sendbyte(reg);
	myi2c_Receiveack();

	myi2c_start();
	myi2c_Sendbyte((daddr<<1)+1);
	myi2c_Receiveack();
	for(i=0;i<length;i++)
	{
		buff[i] = myi2c_Receivebyte();
		if(i<length-1)
		{myi2c_Sendack(0);}
	}
	myi2c_Sendack(1);
	myi2c_end();
	return 0;
}
