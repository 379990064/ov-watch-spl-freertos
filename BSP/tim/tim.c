#include "tim.h"


TIM_TimeBaseInitTypeDef  TIM2_TimeBaseInitStructure;
TIM_OCInitTypeDef        TIM2_OCInitStructure;


void MX_TIM2_Init(void)
{
    GPIO_InitTypeDef GPIO_InitStructure;


    RCC_APB1PeriphClockCmd(RCC_APB1Periph_TIM2, ENABLE);
    RCC_AHB1PeriphClockCmd(RCC_AHB1Periph_GPIOA, ENABLE);
    RCC_APB2PeriphClockCmd(RCC_APB2Periph_SYSCFG, ENABLE);


    GPIO_InitStructure.GPIO_Pin   = GPIO_Pin_15;
    GPIO_InitStructure.GPIO_Mode  = GPIO_Mode_AF;
    GPIO_InitStructure.GPIO_OType = GPIO_OType_PP;
    GPIO_InitStructure.GPIO_PuPd  = GPIO_PuPd_NOPULL;
    GPIO_InitStructure.GPIO_Speed = GPIO_Speed_2MHz;  
    GPIO_Init(GPIOA, &GPIO_InitStructure);

    /* 3. 引脚复用映射到 TIM2 */
    GPIO_PinAFConfig(GPIOA, GPIO_PinSource15, GPIO_AF_TIM2);



    TIM2_TimeBaseInitStructure.TIM_Prescaler         = 99;                      
    TIM2_TimeBaseInitStructure.TIM_CounterMode       = TIM_CounterMode_Up;      
    TIM2_TimeBaseInitStructure.TIM_Period            = 300;                     
    TIM2_TimeBaseInitStructure.TIM_ClockDivision     = TIM_CKD_DIV1;           
    TIM2_TimeBaseInitStructure.TIM_RepetitionCounter = 0;                      
    TIM_TimeBaseInit(TIM2, &TIM2_TimeBaseInitStructure);

    TIM_SelectOutputTrigger(TIM2, TIM_TRGOSource_Reset);   
    TIM_SelectMasterSlaveMode(TIM2, TIM_MasterSlaveMode_Disable);

    /* PWM 通道 1 */
    TIM2_OCInitStructure.TIM_OCMode      = TIM_OCMode_PWM1;          
    TIM2_OCInitStructure.TIM_OutputState = TIM_OutputState_Enable;    
    TIM2_OCInitStructure.TIM_Pulse       = 0;                        
    TIM2_OCInitStructure.TIM_OCPolarity  = TIM_OCPolarity_High;       
    TIM2_OCInitStructure.TIM_OCIdleState = TIM_OCIdleState_Reset;    
    TIM_OC1Init(TIM2, &TIM2_OCInitStructure);

    TIM_OC1PreloadConfig(TIM2, TIM_OCPreload_Disable);
    TIM_ARRPreloadConfig(TIM2, DISABLE);  

   
    TIM_Cmd(TIM2, ENABLE);
}


void TIM2_SetPulse(uint16_t pulse)
{
    TIM_SetCompare1(TIM2, pulse);
}
