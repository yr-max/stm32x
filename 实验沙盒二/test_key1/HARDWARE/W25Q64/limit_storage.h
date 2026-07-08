#ifndef __LIMIT_STORAGE_H
#define __LIMIT_STORAGE_H

#include "stm32f10x.h"

/* 上下限存储地址：W25Q64 扇区 0 起始 */
#define LIMIT_STORAGE_ADDR      0x000000

/* 默认值 */
#define DEFAULT_TEMP_LIMIT      35
#define DEFAULT_LIGHT_LIMIT     500

void LimitStorage_Init(void);
void LimitStorage_Read(int16_t *temp_limit, int16_t *light_limit);
void LimitStorage_Write(int16_t temp_limit, int16_t light_limit);

#endif
