#ifndef __USART_H__
#define __USART_H__

#ifdef __cplusplus
extern "C" {
#endif

#include "stm32f4xx.h"
#include <stdio.h>

void MX_USART1_UART_Init(void);
void USART1_DMA_TX_Start(uint8_t *buf, uint16_t len);
void USART1_DMA_RX_Start(uint8_t *buf, uint16_t len);

/* USART1 DMA 接收缓冲区（供任务/中断使用） */
extern uint8_t HardInt_receive_str[25];

#ifdef __cplusplus
}
#endif

#endif /* __USART_H__ */
