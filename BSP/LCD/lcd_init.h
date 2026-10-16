#ifndef __LCD_INIT_H
#define __LCD_INIT_H

#include "sys.h"

#define USE_HORIZONTAL 0  //���ú�������������ʾ 0��1Ϊ���� 2��3Ϊ����


#if USE_HORIZONTAL==0||USE_HORIZONTAL==1
#define LCD_W 240
#define LCD_H 280

#else
#define LCD_W 280
#define LCD_H 240
#endif


//-----------------LCD�˿ڶ���---------------- 
#define SCLK_PORT			GPIOB
#define SCLK_PIN			GPIO_Pin_3

#define SDA_PORT			GPIOB
#define SDA_PIN				GPIO_Pin_5

#define RES_PORT			GPIOB
#define RES_PIN				GPIO_Pin_4

#define DC_PORT				GPIOC
#define DC_PIN				GPIO_Pin_12

#define CS_PORT				GPIOD
#define CS_PIN				GPIO_Pin_2

#define BLK_PORT			GPIOA
#define BLK_PIN				GPIO_Pin_15

#define LCD_SCLK_Clr() GPIO_WriteBit(SCLK_PORT,SCLK_PIN,RESET)//SCL=SCLK
#define LCD_SCLK_Set() GPIO_WriteBit(SCLK_PORT,SCLK_PIN,SET)

#define LCD_MOSI_Clr() GPIO_WriteBit(SDA_PORT,SDA_PIN,RESET)//SDA=MOSI
#define LCD_MOSI_Set() GPIO_WriteBit(SDA_PORT,SDA_PIN,SET)

#define LCD_RES_Clr()  GPIO_WriteBit(RES_PORT,RES_PIN,RESET)//RES
#define LCD_RES_Set()  GPIO_WriteBit(RES_PORT,RES_PIN,SET)

#define LCD_DC_Clr()  GPIO_WriteBit(DC_PORT,DC_PIN,RESET)//DC
#define LCD_DC_Set()  GPIO_WriteBit(DC_PORT,DC_PIN,SET)
 		     
#define LCD_CS_Clr()   GPIO_WriteBit(CS_PORT,CS_PIN,RESET)//CS
#define LCD_CS_Set()  GPIO_WriteBit(CS_PORT,CS_PIN,SET)

#define LCD_BLK_Clr()  GPIO_WriteBit(BLK_PORT,BLK_PIN,RESET)//BLK
#define LCD_BLK_Set()  GPIO_WriteBit(BLK_PORT,BLK_PIN,SET)

void LCD_GPIO_Init(void);//��ʼ��GPIO
void LCD_Writ_Bus(u8 dat);//ģ��SPIʱ��
void LCD_WR_DATA8(u8 dat);//д��һ���ֽ�
void LCD_WR_DATA(u16 dat);//д�������ֽ�
void LCD_WR_REG(u8 dat);//д��һ��ָ��
void LCD_Address_Set(u16 x1,u16 y1,u16 x2,u16 y2);//�������꺯��
void LCD_Init(void);//LCD��ʼ��
void LCD_Set_Light(uint8_t dc);
void LCD_Close_Light(void);
void LCD_ST7789_SleepIn(void);
void LCD_ST7789_SleepOut(void);
void LCD_Open_Light(void);
#endif




