/**
 * @file    servo.c
 * @brief   舵机驱动：TIM3_CH1, PA6, 50Hz(20ms), 脉冲 500~2500us → 0~180°
 * @note    实验四：基于 6-4 PWM驱动舵机 参考例程移植
 *          适配 STM32F103C8T6 最小系统板（避开 PA1~PA4 W25Q64 冲突）
 */

#include "servo.h"

/**
 * @brief  舵机初始化：TIM3 CH1 → PA6, 20ms 周期 (50Hz)
 */
void Servo_Init(void)
{
    GPIO_InitTypeDef GPIO_InitStructure;
    TIM_TimeBaseInitTypeDef TIM_TimeBaseInitStructure;
    TIM_OCInitTypeDef TIM_OCInitStructure;

    /* 使能时钟 */
    RCC_APB1PeriphClockCmd(RCC_APB1Periph_TIM3, ENABLE);
    RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOA, ENABLE);

    /* PA6 → 复用推挽输出 (TIM3_CH1) */
    GPIO_InitStructure.GPIO_Mode = GPIO_Mode_AF_PP;
    GPIO_InitStructure.GPIO_Pin = GPIO_Pin_6;
    GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
    GPIO_Init(GPIOA, &GPIO_InitStructure);

    /* 时基配置：72MHz / 72 = 1MHz → 计数 20000 = 20ms */
    TIM_InternalClockConfig(TIM3);
    TIM_TimeBaseInitStructure.TIM_ClockDivision = TIM_CKD_DIV1;
    TIM_TimeBaseInitStructure.TIM_CounterMode = TIM_CounterMode_Up;
    TIM_TimeBaseInitStructure.TIM_Period = 20000 - 1;       /* ARR */
    TIM_TimeBaseInitStructure.TIM_Prescaler = 72 - 1;       /* PSC */
    TIM_TimeBaseInitStructure.TIM_RepetitionCounter = 0;
    TIM_TimeBaseInit(TIM3, &TIM_TimeBaseInitStructure);

    /* OC1 通道：PWM1 模式 */
    TIM_OCStructInit(&TIM_OCInitStructure);
    TIM_OCInitStructure.TIM_OCMode = TIM_OCMode_PWM1;
    TIM_OCInitStructure.TIM_OCPolarity = TIM_OCPolarity_High;
    TIM_OCInitStructure.TIM_OutputState = TIM_OutputState_Enable;
    TIM_OCInitStructure.TIM_Pulse = 0;                      /* 初始占空比 0 */
    TIM_OC1Init(TIM3, &TIM_OCInitStructure);

    TIM_Cmd(TIM3, ENABLE);
}

/**
 * @brief   设置舵机角度
 * @param   Angle  0~180 度
 * @note    脉冲映射：0°→500us, 90°→1500us, 180°→2500us
 *          公式：Compare = 500 + Angle * 2000 / 180
 */
void Servo_SetAngle(uint8_t Angle)
{
    uint16_t Compare;

    if (Angle > 180) Angle = 180;

    Compare = 500 + (uint32_t)Angle * 2000 / 180;
    TIM_SetCompare1(TIM3, Compare);
}
