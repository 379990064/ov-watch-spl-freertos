#include "MYSPI.h"


SPI_InitTypeDef SPI1_InitStructure;
DMA_InitTypeDef DMA2_Stream2_InitStructure;

/* SPI1 初始化 */
void MX_SPI1_Init(void)
{
    GPIO_InitTypeDef GPIO_InitStructure;


    RCC_AHB1PeriphClockCmd(RCC_AHB1Periph_GPIOB, ENABLE);
    RCC_APB2PeriphClockCmd(RCC_APB2Periph_SPI1, ENABLE);
    RCC_AHB1PeriphClockCmd(RCC_AHB1Periph_DMA2, ENABLE);


    GPIO_InitStructure.GPIO_Pin   = GPIO_Pin_3 | GPIO_Pin_5;
    GPIO_InitStructure.GPIO_Mode  = GPIO_Mode_AF;
    GPIO_InitStructure.GPIO_OType = GPIO_OType_PP;
    GPIO_InitStructure.GPIO_PuPd  = GPIO_PuPd_NOPULL;
    GPIO_InitStructure.GPIO_Speed = GPIO_Speed_100MHz;
    GPIO_Init(GPIOB, &GPIO_InitStructure);


    GPIO_PinAFConfig(GPIOB, GPIO_PinSource3, GPIO_AF_SPI1);
    GPIO_PinAFConfig(GPIOB, GPIO_PinSource5, GPIO_AF_SPI1);


    SPI1_InitStructure.SPI_Direction         = SPI_Direction_2Lines_FullDuplex;
    SPI1_InitStructure.SPI_Mode              = SPI_Mode_Master;
    SPI1_InitStructure.SPI_DataSize          = SPI_DataSize_8b;
    SPI1_InitStructure.SPI_CPOL              = SPI_CPOL_High;        
    SPI1_InitStructure.SPI_CPHA              = SPI_CPHA_2Edge;       
    SPI1_InitStructure.SPI_NSS               = SPI_NSS_Soft;         
    SPI1_InitStructure.SPI_BaudRatePrescaler = SPI_BaudRatePrescaler_2; 
    SPI1_InitStructure.SPI_FirstBit          = SPI_FirstBit_MSB;     
    SPI1_InitStructure.SPI_CRCPolynomial     = 10;                  
    SPI_Init(SPI1, &SPI1_InitStructure);


    DMA2_Stream2_InitStructure.DMA_Channel            = DMA_Channel_2;
    DMA2_Stream2_InitStructure.DMA_PeripheralBaseAddr = (uint32_t)&(SPI1->DR);
    DMA2_Stream2_InitStructure.DMA_Memory0BaseAddr    = 0;  
    DMA2_Stream2_InitStructure.DMA_DIR                = DMA_DIR_MemoryToPeripheral;
    DMA2_Stream2_InitStructure.DMA_BufferSize         = 0;  
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

    NVIC_InitTypeDef NVIC_InitStructure;

    NVIC_InitStructure.NVIC_IRQChannel                   = DMA2_Stream2_IRQn;
    NVIC_InitStructure.NVIC_IRQChannelPreemptionPriority = 0;
    NVIC_InitStructure.NVIC_IRQChannelSubPriority        = 0;
    NVIC_InitStructure.NVIC_IRQChannelCmd                = ENABLE;
    NVIC_Init(&NVIC_InitStructure);

// 使能 DMA2_Stream2 的中断
    DMA_ITConfig(DMA2_Stream2, DMA_IT_TC, ENABLE);
    SPI_I2S_DMACmd(SPI1, SPI_I2S_DMAReq_Tx, ENABLE);

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

uint8_t SPI1_ReadWriteByte(uint8_t TxData)
{
    while (SPI_I2S_GetFlagStatus(SPI1, SPI_I2S_FLAG_TXE) == RESET);
    SPI_I2S_SendData(SPI1, TxData);
    while (SPI_I2S_GetFlagStatus(SPI1, SPI_I2S_FLAG_RXNE) == RESET);
    return SPI_I2S_ReceiveData(SPI1);
}

void SPI1_TransmitByte(uint8_t dat)
{
    while (SPI_I2S_GetFlagStatus(SPI1, SPI_I2S_FLAG_TXE) == RESET);
    SPI_I2S_SendData(SPI1, dat);
    while (SPI_I2S_GetFlagStatus(SPI1, SPI_I2S_FLAG_BSY) == SET);
}
