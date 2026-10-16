#include "serial.h"
#include "stm32f4xx.h"
/* USART1 初始化结构体 */
USART_InitTypeDef USART1_InitStructure;

/* DMA 收发结构体 */
DMA_InitTypeDef DMA2_Stream5_InitStructure;  // USART1_RX
DMA_InitTypeDef DMA2_Stream7_InitStructure;  // USART1_TX

/* USART1 DMA 接收缓冲区（25 字节，与 USART1_DMA_RX_Start 长度一致） */
uint8_t HardInt_receive_str[25] = {0};

/* USART1 初始化函数（对应 MX_USART1_UART_Init） */
void MX_USART1_UART_Init(void)
{
    GPIO_InitTypeDef GPIO_InitStructure;

    /* 1. 使能时钟：GPIOA、USART1、DMA2 */
    RCC_AHB1PeriphClockCmd(RCC_AHB1Periph_GPIOA, ENABLE);
    RCC_APB2PeriphClockCmd(RCC_APB2Periph_USART1, ENABLE);
    RCC_AHB1PeriphClockCmd(RCC_AHB1Periph_DMA2, ENABLE);

    /* 2. 配置 USART1 引脚：PA9=TX, PA10=RX */
    GPIO_InitStructure.GPIO_Pin   = GPIO_Pin_9 | GPIO_Pin_10;
    GPIO_InitStructure.GPIO_Mode  = GPIO_Mode_AF;
    GPIO_InitStructure.GPIO_OType = GPIO_OType_PP;
    GPIO_InitStructure.GPIO_PuPd  = GPIO_PuPd_NOPULL;
    GPIO_InitStructure.GPIO_Speed = GPIO_Speed_100MHz;
    GPIO_Init(GPIOA, &GPIO_InitStructure);

    /* 3. 引脚复用映射到 USART1 */
    GPIO_PinAFConfig(GPIOA, GPIO_PinSource9,  GPIO_AF_USART1);
    GPIO_PinAFConfig(GPIOA, GPIO_PinSource10, GPIO_AF_USART1);

    /* 4. USART1 参数配置（完全对应 HAL 参数） */
    USART1_InitStructure.USART_BaudRate            = 115200;
    USART1_InitStructure.USART_WordLength          = USART_WordLength_8b;
    USART1_InitStructure.USART_StopBits            = USART_StopBits_1;
    USART1_InitStructure.USART_Parity              = USART_Parity_No;
    USART1_InitStructure.USART_Mode                = USART_Mode_Rx | USART_Mode_Tx;
    USART1_InitStructure.USART_HardwareFlowControl = USART_HardwareFlowControl_None;
    USART_Init(USART1, &USART1_InitStructure);

    /* 5. 配置 DMA2 Stream5 Channel4（USART1_RX） */
    DMA2_Stream5_InitStructure.DMA_Channel            = DMA_Channel_4;
    DMA2_Stream5_InitStructure.DMA_PeripheralBaseAddr = (uint32_t)&(USART1->DR);
    DMA2_Stream5_InitStructure.DMA_Memory0BaseAddr    = 0;  // 接收时动态设置
    DMA2_Stream5_InitStructure.DMA_DIR                = DMA_DIR_PeripheralToMemory;
    DMA2_Stream5_InitStructure.DMA_BufferSize         = 0;  // 接收时动态设置
    DMA2_Stream5_InitStructure.DMA_PeripheralInc      = DMA_PeripheralInc_Disable;
    DMA2_Stream5_InitStructure.DMA_MemoryInc          = DMA_MemoryInc_Enable;
    DMA2_Stream5_InitStructure.DMA_PeripheralDataSize = DMA_PeripheralDataSize_Byte;
    DMA2_Stream5_InitStructure.DMA_MemoryDataSize     = DMA_MemoryDataSize_Byte;
    DMA2_Stream5_InitStructure.DMA_Mode               = DMA_Mode_Normal;
    DMA2_Stream5_InitStructure.DMA_Priority           = DMA_Priority_Low;
    DMA2_Stream5_InitStructure.DMA_FIFOMode           = DMA_FIFOMode_Disable;
    DMA2_Stream5_InitStructure.DMA_FIFOThreshold      = DMA_FIFOThreshold_HalfFull;
    DMA2_Stream5_InitStructure.DMA_MemoryBurst        = DMA_MemoryBurst_Single;
    DMA2_Stream5_InitStructure.DMA_PeripheralBurst    = DMA_PeripheralBurst_Single;
    DMA_Init(DMA2_Stream5, &DMA2_Stream5_InitStructure);

    /* 6. 配置 DMA2 Stream7 Channel4（USART1_TX） */
    DMA2_Stream7_InitStructure.DMA_Channel            = DMA_Channel_4;
    DMA2_Stream7_InitStructure.DMA_PeripheralBaseAddr = (uint32_t)&(USART1->DR);
    DMA2_Stream7_InitStructure.DMA_Memory0BaseAddr    = 0;  // 发送时动态设置
    DMA2_Stream7_InitStructure.DMA_DIR                = DMA_DIR_MemoryToPeripheral;
    DMA2_Stream7_InitStructure.DMA_BufferSize         = 0;  // 发送时动态设置
    DMA2_Stream7_InitStructure.DMA_PeripheralInc      = DMA_PeripheralInc_Disable;
    DMA2_Stream7_InitStructure.DMA_MemoryInc          = DMA_MemoryInc_Enable;
    DMA2_Stream7_InitStructure.DMA_PeripheralDataSize = DMA_PeripheralDataSize_Byte;
    DMA2_Stream7_InitStructure.DMA_MemoryDataSize     = DMA_MemoryDataSize_Byte;
    DMA2_Stream7_InitStructure.DMA_Mode               = DMA_Mode_Normal;
    DMA2_Stream7_InitStructure.DMA_Priority           = DMA_Priority_Low;
    DMA2_Stream7_InitStructure.DMA_FIFOMode           = DMA_FIFOMode_Disable;
    DMA2_Stream7_InitStructure.DMA_FIFOThreshold      = DMA_FIFOThreshold_HalfFull;
    DMA2_Stream7_InitStructure.DMA_MemoryBurst        = DMA_MemoryBurst_Single;
    DMA2_Stream7_InitStructure.DMA_PeripheralBurst    = DMA_PeripheralBurst_Single;
    DMA_Init(DMA2_Stream7, &DMA2_Stream7_InitStructure);

    /* 7. 使能 USART1 的 DMA 收发请求 */
    USART_DMACmd(USART1, USART_DMAReq_Tx | USART_DMAReq_Rx, ENABLE);

    /* 8. 使能 USART1 */
    USART_Cmd(USART1, ENABLE);

    /* 9. 配置 NVIC（USART1 中断） */
    {
        NVIC_InitTypeDef NVIC_InitStructure;
        NVIC_InitStructure.NVIC_IRQChannel                   = USART1_IRQn;
        NVIC_InitStructure.NVIC_IRQChannelPreemptionPriority = 5;
        NVIC_InitStructure.NVIC_IRQChannelSubPriority        = 0;
        NVIC_InitStructure.NVIC_IRQChannelCmd                = ENABLE;
        NVIC_Init(&NVIC_InitStructure);
    }
}

/* 使用 DMA 发送一段数据 */
void USART1_DMA_TX_Start(uint8_t *buf, uint16_t len)
{
    while (DMA_GetCmdStatus(DMA2_Stream7) != DISABLE);

    DMA_Cmd(DMA2_Stream7, DISABLE);
    DMA_ClearFlag(DMA2_Stream7, DMA_FLAG_TCIF7 | DMA_FLAG_HTIF7 | DMA_FLAG_TEIF7 |
                                DMA_FLAG_DMEIF7 | DMA_FLAG_FEIF7);
    DMA2_Stream7->M0AR = (uint32_t)buf;
    DMA_SetCurrDataCounter(DMA2_Stream7, len);
    DMA_Cmd(DMA2_Stream7, ENABLE);
}

/* 使用 DMA 接收一段数据 */
void USART1_DMA_RX_Start(uint8_t *buf, uint16_t len)
{
    while (DMA_GetCmdStatus(DMA2_Stream5) != DISABLE);

    DMA_Cmd(DMA2_Stream5, DISABLE);
    DMA_ClearFlag(DMA2_Stream5, DMA_FLAG_TCIF5 | DMA_FLAG_HTIF5 | DMA_FLAG_TEIF5 |
                                DMA_FLAG_DMEIF5 | DMA_FLAG_FEIF5);
    DMA2_Stream5->M0AR = (uint32_t)buf;
    DMA_SetCurrDataCounter(DMA2_Stream5, len);
    DMA_Cmd(DMA2_Stream5, ENABLE);
}

/* fputc 重定向（printf 输出到 USART1） */
int fputc(int ch, FILE *f)
{
    while (USART_GetFlagStatus(USART1, USART_FLAG_TXE) == RESET);
    USART_SendData(USART1, (uint8_t)ch);
    while (USART_GetFlagStatus(USART1, USART_FLAG_TC) == RESET);
    return ch;
}
