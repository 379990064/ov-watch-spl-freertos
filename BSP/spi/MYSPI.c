#include "MYSPI.h"

/* SPI1 全局句柄（标准库无句柄，但保留结构体方便扩展） */
SPI_InitTypeDef SPI1_InitStructure;
DMA_InitTypeDef DMA2_Stream2_InitStructure;

/* SPI1 初始化函数（对应 MX_SPI1_Init） */
void MX_SPI1_Init(void)
{
    GPIO_InitTypeDef GPIO_InitStructure;

    /* 1. 使能时钟：GPIOB、SPI1、DMA2 */
    RCC_AHB1PeriphClockCmd(RCC_AHB1Periph_GPIOB, ENABLE);
    RCC_APB2PeriphClockCmd(RCC_APB2Periph_SPI1, ENABLE);
    RCC_AHB1PeriphClockCmd(RCC_AHB1Periph_DMA2, ENABLE);

    /* 2. 配置 SPI1 引脚：PB3=SCK, PB5=MOSI */
    GPIO_InitStructure.GPIO_Pin   = GPIO_Pin_3 | GPIO_Pin_5;
    GPIO_InitStructure.GPIO_Mode  = GPIO_Mode_AF;
    GPIO_InitStructure.GPIO_OType = GPIO_OType_PP;
    GPIO_InitStructure.GPIO_PuPd  = GPIO_PuPd_NOPULL;
    GPIO_InitStructure.GPIO_Speed = GPIO_Speed_100MHz;
    GPIO_Init(GPIOB, &GPIO_InitStructure);

    /* 3. 引脚复用映射到 SPI1 */
    GPIO_PinAFConfig(GPIOB, GPIO_PinSource3, GPIO_AF_SPI1);
    GPIO_PinAFConfig(GPIOB, GPIO_PinSource5, GPIO_AF_SPI1);

    /* 4. SPI1 参数配置（完全对应 HAL 参数） */
    SPI1_InitStructure.SPI_Direction         = SPI_Direction_2Lines_FullDuplex;
    SPI1_InitStructure.SPI_Mode              = SPI_Mode_Master;
    SPI1_InitStructure.SPI_DataSize          = SPI_DataSize_8b;
    SPI1_InitStructure.SPI_CPOL              = SPI_CPOL_High;        // 对应 SPI_POLARITY_HIGH
    SPI1_InitStructure.SPI_CPHA              = SPI_CPHA_2Edge;       // 对应 SPI_PHASE_2EDGE
    SPI1_InitStructure.SPI_NSS               = SPI_NSS_Soft;         // 对应 SPI_NSS_SOFT
    SPI1_InitStructure.SPI_BaudRatePrescaler = SPI_BaudRatePrescaler_2; // 对应 PRESCALER_2
    SPI1_InitStructure.SPI_FirstBit          = SPI_FirstBit_MSB;     // 对应 FIRSTBIT_MSB
    SPI1_InitStructure.SPI_CRCPolynomial     = 10;                   // 对应 CRCPolynomial = 10
    SPI_Init(SPI1, &SPI1_InitStructure);

    /* 5. 配置 DMA2 Stream2 Channel2（SPI1_TX） */
    DMA2_Stream2_InitStructure.DMA_Channel            = DMA_Channel_2;
    DMA2_Stream2_InitStructure.DMA_PeripheralBaseAddr = (uint32_t)&(SPI1->DR);
    DMA2_Stream2_InitStructure.DMA_Memory0BaseAddr    = 0;  // 发送时动态设置
    DMA2_Stream2_InitStructure.DMA_DIR                = DMA_DIR_MemoryToPeripheral;
    DMA2_Stream2_InitStructure.DMA_BufferSize         = 0;  // 发送时动态设置
    DMA2_Stream2_InitStructure.DMA_PeripheralInc      = DMA_PeripheralInc_Disable;
    DMA2_Stream2_InitStructure.DMA_MemoryInc          = DMA_MemoryInc_Enable;
    DMA2_Stream2_InitStructure.DMA_PeripheralDataSize = DMA_PeripheralDataSize_Byte;
    DMA2_Stream2_InitStructure.DMA_MemoryDataSize     = DMA_MemoryDataSize_Byte;
    DMA2_Stream2_InitStructure.DMA_Mode               = DMA_Mode_Normal;
    DMA2_Stream2_InitStructure.DMA_Priority           = DMA_Priority_Low;
    DMA2_Stream2_InitStructure.DMA_FIFOMode           = DMA_FIFOMode_Disable;
    DMA2_Stream2_InitStructure.DMA_FIFOThreshold      = DMA_FIFOThreshold_HalfFull;
    DMA2_Stream2_InitStructure.DMA_MemoryBurst        = DMA_MemoryBurst_Single;
    DMA2_Stream2_InitStructure.DMA_PeripheralBurst    = DMA_PeripheralBurst_Single;
    DMA_Init(DMA2_Stream2, &DMA2_Stream2_InitStructure);

    /* 6. 使能 SPI1 的 TX DMA 请求 */
    SPI_I2S_DMACmd(SPI1, SPI_I2S_DMAReq_Tx, ENABLE);

    /* 7. 使能 SPI1 */
    SPI_Cmd(SPI1, ENABLE);
}

/* 使用 DMA 发送一段数据 */
void SPI1_DMA_TX_Start(uint8_t *buf, uint16_t len)
{
    /* 等待 DMA 空闲 */
    while (DMA_GetCmdStatus(DMA2_Stream2) != DISABLE);

    /* 关闭 DMA，重新配置地址和长度 */
    DMA_Cmd(DMA2_Stream2, DISABLE);
    DMA_ClearFlag(DMA2_Stream2, DMA_FLAG_TCIF2 | DMA_FLAG_HTIF2 | DMA_FLAG_TEIF2 |
                                DMA_FLAG_DMEIF2 | DMA_FLAG_FEIF2);
    DMA2_Stream2->M0AR = (uint32_t)buf;
    DMA_SetCurrDataCounter(DMA2_Stream2, len);
    DMA_Cmd(DMA2_Stream2, ENABLE);
}