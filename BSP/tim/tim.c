#include "tim.h"

/* TIM2 初始化结构体 */
TIM_TimeBaseInitTypeDef  TIM2_TimeBaseInitStructure;
TIM_OCInitTypeDef        TIM2_OCInitStructure;

/* TIM2 初始化函数（对应 MX_TIM2_Init） */
void MX_TIM2_Init(void)
{
    GPIO_InitTypeDef GPIO_InitStructure;

    /* 1. 使能时钟：TIM2、GPIOA、SYSCFG */
    RCC_APB1PeriphClockCmd(RCC_APB1Periph_TIM2, ENABLE);
    RCC_AHB1PeriphClockCmd(RCC_AHB1Periph_GPIOA, ENABLE);
    RCC_APB2PeriphClockCmd(RCC_APB2Periph_SYSCFG, ENABLE);

    /* 2. 配置 PA15 为复用模式（TIM2_CH1） */
    GPIO_InitStructure.GPIO_Pin   = GPIO_Pin_15;
    GPIO_InitStructure.GPIO_Mode  = GPIO_Mode_AF;
    GPIO_InitStructure.GPIO_OType = GPIO_OType_PP;
    GPIO_InitStructure.GPIO_PuPd  = GPIO_PuPd_NOPULL;
    GPIO_InitStructure.GPIO_Speed = GPIO_Speed_2MHz;  // 对应 LOW
    GPIO_Init(GPIOA, &GPIO_InitStructure);

    /* 3. 引脚复用映射到 TIM2 */
    GPIO_PinAFConfig(GPIOA, GPIO_PinSource15, GPIO_AF_TIM2);


    /* 5. TIM2 时基配置（完全对应 HAL 参数） */
    TIM2_TimeBaseInitStructure.TIM_Prescaler         = 99;                      // 对应 Prescaler
    TIM2_TimeBaseInitStructure.TIM_CounterMode       = TIM_CounterMode_Up;      // 对应 TIM_COUNTERMODE_UP
    TIM2_TimeBaseInitStructure.TIM_Period            = 300;                     // 对应 Period
    TIM2_TimeBaseInitStructure.TIM_ClockDivision     = TIM_CKD_DIV1;            // 对应 TIM_CLOCKDIVISION_DIV1
    TIM2_TimeBaseInitStructure.TIM_RepetitionCounter = 0;                       // 对应 AutoReloadPreload 相关（TIM2 无 RCR）
    TIM_TimeBaseInit(TIM2, &TIM2_TimeBaseInitStructure);

    /* 6. 主从模式配置（对应 TIM_TRGO_RESET + TIM_MASTERSLAVEMODE_DISABLE） */
    TIM_SelectOutputTrigger(TIM2, TIM_TRGOSource_Reset);   // TRGO = RESET
    TIM_SelectMasterSlaveMode(TIM2, TIM_MasterSlaveMode_Disable);

    /* 7. PWM 通道 1 配置（完全对应 HAL 参数） */
    TIM2_OCInitStructure.TIM_OCMode      = TIM_OCMode_PWM1;           // 对应 TIM_OCMODE_PWM1
    TIM2_OCInitStructure.TIM_OutputState = TIM_OutputState_Enable;    // 对应使能输出
    TIM2_OCInitStructure.TIM_Pulse       = 0;                         // 对应 Pulse = 0
    TIM2_OCInitStructure.TIM_OCPolarity  = TIM_OCPolarity_High;       // 对应 TIM_OCPOLARITY_HIGH
    TIM2_OCInitStructure.TIM_OCIdleState = TIM_OCIdleState_Reset;     // 空闲状态（高级定时器用，这里保留默认）
    TIM_OC1Init(TIM2, &TIM2_OCInitStructure);

    /* 8. 使能预装载（对应 AutoReloadPreload 相关） */
    TIM_OC1PreloadConfig(TIM2, TIM_OCPreload_Disable);
    TIM_ARRPreloadConfig(TIM2, DISABLE);  // 对应 TIM_AUTORELOAD_PRELOAD_DISABLE

    /* 9. 使能 TIM2 */
    TIM_Cmd(TIM2, ENABLE);
}

/* 修改 PWM 占空比（对应 __HAL_TIM_SET_COMPARE） */
void TIM2_SetPulse(uint16_t pulse)
{
    TIM_SetCompare1(TIM2, pulse);
}
