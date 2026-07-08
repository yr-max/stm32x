/**
 * @file    motor.c
 * @brief   直流电机驱动：TIM2_CH3 (PB10) PWM + PB8/PB9 方向控制 (TB6612FNG)
 * @note    实验四：基于 6-5 PWM驱动直流电机 参考例程移植
 *          适配 STM32F103C8T6 + TB6612FNG 电机驱动模块
 *          PB8 = AIN1, PB9 = AIN2, PB10 = PWMA
 *          TIM2_CH3 默认在 PA2，需 PartialRemap2 映射到 PB10
 */

#include "motor.h"

/* 引脚宏 */
#define MOTOR_IN1_PORT  GPIOB
#define MOTOR_IN1_PIN   GPIO_Pin_8

#define MOTOR_IN2_PORT  GPIOB
#define MOTOR_IN2_PIN   GPIO_Pin_9

/* PWM 参数：72MHz / 72 = 1MHz, 1MHz / 100 = 10kHz */
#define PWM_ARR  (100 - 1)
#define PWM_PSC  (72 - 1)

/**
 * @brief  电机初始化：TIM2 CH3 → PB10, IN1=PB8, IN2=PB9
 */
void Motor_Init(void)
{
    GPIO_InitTypeDef GPIO_InitStructure;
    TIM_TimeBaseInitTypeDef TIM_TimeBaseInitStructure;
    TIM_OCInitTypeDef TIM_OCInitStructure;

    /* 使能时钟 */
    RCC_APB1PeriphClockCmd(RCC_APB1Periph_TIM2, ENABLE);
    RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOB, ENABLE);
    RCC_APB2PeriphClockCmd(RCC_APB2Periph_AFIO, ENABLE);
    GPIO_PinRemapConfig(GPIO_PartialRemap2_TIM2, ENABLE);  /* TIM2_CH3 → PB10 */

    /* PB8, PB9 → 推挽输出 (方向控制) */
    GPIO_InitStructure.GPIO_Mode = GPIO_Mode_Out_PP;
    GPIO_InitStructure.GPIO_Pin = GPIO_Pin_8 | GPIO_Pin_9;
    GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
    GPIO_Init(GPIOB, &GPIO_InitStructure);

    /* PB10 → 复用推挽输出 (TIM2_CH3) */
    GPIO_InitStructure.GPIO_Mode = GPIO_Mode_AF_PP;
    GPIO_InitStructure.GPIO_Pin = GPIO_Pin_10;
    GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
    GPIO_Init(GPIOB, &GPIO_InitStructure);

    /* 时基配置：10kHz PWM */
    TIM_InternalClockConfig(TIM2);
    TIM_TimeBaseInitStructure.TIM_ClockDivision = TIM_CKD_DIV1;
    TIM_TimeBaseInitStructure.TIM_CounterMode = TIM_CounterMode_Up;
    TIM_TimeBaseInitStructure.TIM_Period = PWM_ARR;
    TIM_TimeBaseInitStructure.TIM_Prescaler = PWM_PSC;
    TIM_TimeBaseInitStructure.TIM_RepetitionCounter = 0;
    TIM_TimeBaseInit(TIM2, &TIM_TimeBaseInitStructure);

    /* OC3 通道：PWM1 模式 */
    TIM_OCStructInit(&TIM_OCInitStructure);
    TIM_OCInitStructure.TIM_OCMode = TIM_OCMode_PWM1;
    TIM_OCInitStructure.TIM_OCPolarity = TIM_OCPolarity_High;
    TIM_OCInitStructure.TIM_OutputState = TIM_OutputState_Enable;
    TIM_OCInitStructure.TIM_Pulse = 0;                      /* 初始占空比 0 */
    TIM_OC3Init(TIM2, &TIM_OCInitStructure);

    TIM_Cmd(TIM2, ENABLE);

    /* 初始化状态：停止 */
    GPIO_ResetBits(MOTOR_IN1_PORT, MOTOR_IN1_PIN);
    GPIO_ResetBits(MOTOR_IN2_PORT, MOTOR_IN2_PIN);
}

/**
 * @brief   设置电机速度与方向
 * @param   Speed  -100 ~ 100
 *           > 0: 正转, = 0: 停止, < 0: 反转
 * @note    TB6612FNG 真值表：
 *          IN1=1 IN2=0 → 正转; IN1=0 IN2=1 → 反转; IN1=0 IN2=0 → 停止
 */
void Motor_SetSpeed(int16_t Speed)
{
    if (Speed > 100)  Speed = 100;
    if (Speed < -100) Speed = -100;

    if (Speed > 0)
    {
        /* 正转 */
        GPIO_SetBits(MOTOR_IN1_PORT, MOTOR_IN1_PIN);
        GPIO_ResetBits(MOTOR_IN2_PORT, MOTOR_IN2_PIN);
        TIM_SetCompare3(TIM2, (uint16_t)Speed);
    }
    else if (Speed < 0)
    {
        /* 反转 */
        GPIO_ResetBits(MOTOR_IN1_PORT, MOTOR_IN1_PIN);
        GPIO_SetBits(MOTOR_IN2_PORT, MOTOR_IN2_PIN);
        TIM_SetCompare3(TIM2, (uint16_t)(-Speed));
    }
    else
    {
        /* 停止 */
        GPIO_ResetBits(MOTOR_IN1_PORT, MOTOR_IN1_PIN);
        GPIO_ResetBits(MOTOR_IN2_PORT, MOTOR_IN2_PIN);
        TIM_SetCompare3(TIM2, 0);
    }
}
