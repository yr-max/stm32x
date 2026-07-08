#include "limit_storage.h"
#include "W25Q64.h"
#include "usart.h"
#include "stdio.h"

/* 存储结构：
 * 字节 0-1：温度上限（int16_t，小端）
 * 字节 2-3：光照上限（int16_t，小端）
 * 字节 4-5：校验和（温度上限 + 光照上限）
 * 字节 6-7：保留对齐
 */

typedef struct
{
    int16_t temp_limit;
    int16_t light_limit;
    int16_t checksum;
    int16_t reserved;
} LimitStorage_t;

/**************************************************************
 * @brief  校验上下限是否合法
 **************************************************************/
static uint8_t LimitStorage_IsValid(LimitStorage_t *pStorage)
{
    if (pStorage == NULL) return 0;
    if (pStorage->checksum != (int16_t)(pStorage->temp_limit + pStorage->light_limit))
        return 0;
    if (pStorage->temp_limit < -40 || pStorage->temp_limit > 125)
        return 0;
    if (pStorage->light_limit < 0 || pStorage->light_limit > 4095)
        return 0;
    return 1;
}

/**************************************************************
 * @brief  初始化 W25Q64 并尝试读取上下限
 **************************************************************/
void LimitStorage_Init(void)
{
    uint8_t MID;
    uint16_t DID;

    W25Q64_Init();
    W25Q64_ReadID(&MID, &DID);

    printf("W25Q64 ID: MID=0x%02X, DID=0x%04X\r\n", MID, DID);
}

/**************************************************************
 * @brief  从 W25Q64 读取温度上限和光照上限
 * @param  temp_limit  输出温度上限
 * @param  light_limit 输出光照上限
 * @note   若读取失败或校验不通过，则使用默认值
 **************************************************************/
void LimitStorage_Read(int16_t *temp_limit, int16_t *light_limit)
{
    LimitStorage_t storage;

    W25Q64_ReadData(LIMIT_STORAGE_ADDR, (uint8_t *)&storage, sizeof(LimitStorage_t));

    if (LimitStorage_IsValid(&storage))
    {
        *temp_limit = storage.temp_limit;
        *light_limit = storage.light_limit;
        printf("Limit read OK: T=%d, L=%d\r\n", *temp_limit, *light_limit);
    }
    else
    {
        *temp_limit = DEFAULT_TEMP_LIMIT;
        *light_limit = DEFAULT_LIGHT_LIMIT;
        printf("Limit invalid, use default: T=%d, L=%d\r\n", *temp_limit, *light_limit);
    }
}

/**************************************************************
 * @brief  将温度上限和光照上限写入 W25Q64
 * @param  temp_limit  温度上限
 * @param  light_limit 光照上限
 **************************************************************/
void LimitStorage_Write(int16_t temp_limit, int16_t light_limit)
{
    LimitStorage_t storage;

    storage.temp_limit = temp_limit;
    storage.light_limit = light_limit;
    storage.checksum = (int16_t)(temp_limit + light_limit);
    storage.reserved = 0;

    W25Q64_SectorErase(LIMIT_STORAGE_ADDR);
    W25Q64_PageProgram(LIMIT_STORAGE_ADDR, (uint8_t *)&storage, sizeof(LimitStorage_t));

    printf("Limit write: T=%d, L=%d\r\n", temp_limit, light_limit);
}
