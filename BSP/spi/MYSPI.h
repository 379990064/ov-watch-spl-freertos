#ifndef __SPI_H__
#define __SPI_H__

#ifdef __cplusplus
extern "C" {
#endif

#include "stm32f4xx.h"

/* 函数声明 */
void MX_SPI1_Init(void);
void SPI1_DMA_TX_Start(uint8_t *buf, uint16_t len);
uint8_t SPI1_ReadWriteByte(uint8_t TxData);
void SPI1_TransmitByte(uint8_t dat);
#ifdef __cplusplus
}
#endif

#endif /* __SPI_H__ */
