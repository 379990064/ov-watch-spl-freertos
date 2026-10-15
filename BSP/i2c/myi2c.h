#ifndef __myi2c_H
#define __myi2c_H


void myi2c_Init(void);
void myi2c_start(void);
void myi2c_end(void);
void myi2c_Sendbyte(uint8_t byte);
uint8_t myi2c_Receivebyte(void);
void myi2c_Sendack(uint8_t ack);
uint8_t myi2c_Receiveack(void);
uint8_t IIC_Read_Multi_Byte(uint8_t daddr, uint8_t reg, uint8_t length, uint8_t buff[]);
uint8_t IIC_Write_Multi_Byte(uint8_t daddr,uint8_t reg,uint8_t length,uint8_t buff[]);

#endif
