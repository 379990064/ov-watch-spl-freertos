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

/* ★ ISR 里安全地重新武装 DMA 接收 + 取已收字节数 */
void     USART1_DMA_RX_Restart_ISR(uint8_t *buf, uint16_t len);
uint16_t USART1_DMA_RX_Count(uint16_t total);

/* ★ UART 收帧标志（ISR 置位，MessageSendTask 轮询并清零） */
extern volatile uint8_t HardInt_uart_flag;

/* ★ 调试输出（直接写 USART1 寄存器，不经过 printf）
 *   本工程用 MicroLIB 且没有 _sys_exit/_ttywrch/__use_no_semihosting 重定向，
 *   printf 会走到半主机 BKPT 上把程序卡死，所以调试信息统一用这三个函数。
 *   实现放在 User/main.c。 */
void dbg_send(uint8_t c);
void dbg_puts(const char *s);
void dbg_putnum(int32_t v);

/* USART1 DMA 接收缓冲区（供任务/中断使用） */
extern uint8_t HardInt_receive_str[25];

#ifdef __cplusplus
}
#endif

#endif /* __USART_H__ */
