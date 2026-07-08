#ifndef __AD_H
#define __AD_H

#include "stm32f10x.h"

void AD_Init(void);
uint16_t AD_GetValue(uint8_t ADC_Channel);

/* 通道定义 */
#define ADC_CH_TEMP   ADC_Channel_0   /* PA0: 热敏电阻传感器 */
#define ADC_CH_LIGHT  ADC_Channel_5   /* PA5: 光敏电阻传感器 */

#endif
