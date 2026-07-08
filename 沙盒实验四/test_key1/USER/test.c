#include "stm32f10x.h"
#include "los_sys.h"
#include "los_typedef.h"
#include "los_task.ph"
#include "los_task.h"
#include "los_mux.h"

#include "sys.h"
#include "led.h"
#include "usart.h"
#include "key.h"
#include "OLED.h"
#include "limit_storage.h"
#include "AD.h"
#include "servo.h"
#include "motor.h"
#include <math.h>

/* 共享数据 */
int16_t g_temp = 25;           /* 当前温度（°C），PA0 热敏 */
int16_t g_light = 50;          /* 当前光照（%），PA5 光敏 */
int16_t g_temp_limit = 35;     /* 温度上限（°C） */
int16_t g_light_limit = 50;    /* 光照上限（%） */
uint8_t g_storage_changed = 0; /* 上下限变化标志，需要保存 */

uint8_t g_servo_angle = 0;     /* 舵机当前角度（0~180°） */
int16_t g_motor_speed = 0;     /* 电机当前转速（-100~100） */

UINT32 g_data_mux;             /* 共享数据互斥量 */

/* 任务句柄 */
UINT32 Task_Display_Handle;
UINT32 Task_Key_Handle;
UINT32 Task_Storage_Handle;
UINT32 Task_Sensor_Handle;

/* 函数声明 */
static UINT32 AppTaskCreate(void);
static UINT32 Create_Task(UINT32 *handle, TSK_ENTRY_FUNC entry, const char *name, uint16_t prio);
static void Task_Display(void);
static void Task_Key(void);
static void Task_Storage(void);
static void Task_Sensor(void);
static void BSP_Init(void);

/***************************************************************
* @brief 主函数
**************************************************************/
int main(void)
{
    UINT32 uwRet = LOS_OK;

    Stm32_Clock_Init(9);    /* 系统时钟 72MHz */
    BSP_Init();

    printf("LiteOS Exp4 Servo-Motor-PWM Start\r\n");

    /* LiteOS 内核初始化 */
    uwRet = LOS_KernelInit();
    if (uwRet != LOS_OK)
    {
        printf("LiteOS 核心初始化失败！0x%X\r\n", uwRet);
        return LOS_NOK;
    }

    /* 创建互斥量保护共享数据 */
    uwRet = LOS_MuxCreate(&g_data_mux);
    if (uwRet != LOS_OK)
    {
        printf("互斥量创建失败！0x%X\r\n", uwRet);
        return LOS_NOK;
    }

    /* 创建应用任务 */
    uwRet = AppTaskCreate();
    if (uwRet != LOS_OK)
    {
        printf("AppTaskCreate 失败！0x%X\r\n", uwRet);
        return LOS_NOK;
    }

    LOS_Start();

    while (1);
}

/**************************************************************
* @brief  创建所有应用任务
**************************************************************/
static UINT32 AppTaskCreate(void)
{
    UINT32 uwRet = LOS_OK;

    uwRet = Create_Task(&Task_Display_Handle, (TSK_ENTRY_FUNC)Task_Display, "Display", 5);
    if (uwRet != LOS_OK) return uwRet;

    uwRet = Create_Task(&Task_Key_Handle, (TSK_ENTRY_FUNC)Task_Key, "Key", 4);
    if (uwRet != LOS_OK) return uwRet;

    uwRet = Create_Task(&Task_Storage_Handle, (TSK_ENTRY_FUNC)Task_Storage, "Storage", 6);
    if (uwRet != LOS_OK) return uwRet;

    uwRet = Create_Task(&Task_Sensor_Handle, (TSK_ENTRY_FUNC)Task_Sensor, "Sensor", 5);
    if (uwRet != LOS_OK) return uwRet;

    return LOS_OK;
}

/**************************************************************
* @brief  通用任务创建辅助函数
**************************************************************/
static UINT32 Create_Task(UINT32 *handle, TSK_ENTRY_FUNC entry, const char *name, uint16_t prio)
{
    TSK_INIT_PARAM_S task_init_param;

    task_init_param.usTaskPrio = prio;
    task_init_param.pcName = (char *)name;
    task_init_param.pfnTaskEntry = entry;
    task_init_param.uwStackSize = 1024;

    return LOS_TaskCreate(handle, &task_init_param);
}

/**************************************************************
* @brief  显示任务：初始化 OLED 并循环刷新
**************************************************************/
static void Task_Display(void)
{
    char buf[32];

    OLED_Init();

    while (1)
    {
        OLED_ResetOffset();         /* 重置起始行+偏移，防 GPIOB 干扰 */

        LOS_MuxPend(g_data_mux, LOS_WAIT_FOREVER);

        sprintf(buf, "T:%-3dC L:%-3d%%", g_temp, g_light);
        OLED_ShowString(1, 1, buf);

        sprintf(buf, "TL:%-3dC LL:%-3d%%", g_temp_limit, g_light_limit);
        OLED_ShowString(2, 1, buf);

        sprintf(buf, "S:%-3d M:%-4d", g_servo_angle, g_motor_speed);
        OLED_ShowString(3, 1, buf);

        if (g_storage_changed)
        {
            OLED_ShowString(4, 1, "Saving...    ");
        }
        else
        {
            OLED_ShowString(4, 1, "Saved OK     ");
        }

        LOS_MuxPost(g_data_mux);

        LOS_TaskDelay(200);     /* 5Hz */
    }
}

/**************************************************************
* @brief  按键任务：短按加，长按减
**************************************************************/
static void Task_Key(void)
{
    uint16_t key0_cnt = 0;
    uint16_t key1_cnt = 0;

    while (1)
    {
        /* KEY0：短按温度上限 +1，长按 1s 后每 100ms -1 */
        if (KEY0 == 0)
        {
            key0_cnt++;
            if (key0_cnt == 2)          /* 40ms 消抖后首次确认 */
            {
                LOS_MuxPend(g_data_mux, LOS_WAIT_FOREVER);
                g_temp_limit++;
                g_storage_changed = 1;
                LOS_MuxPost(g_data_mux);

                LED0 = !LED0;
                printf("TempLimit = %d C\r\n", g_temp_limit);
            }
            else if (key0_cnt == 50)    /* 长按约 1s */
            {
                LOS_MuxPend(g_data_mux, LOS_WAIT_FOREVER);
                g_temp_limit--;
                g_storage_changed = 1;
                LOS_MuxPost(g_data_mux);

                LED0 = !LED0;
                printf("TempLimit = %d C\r\n", g_temp_limit);
                key0_cnt = 45;          /* 之后每 100ms 再减 */
            }
        }
        else
        {
            key0_cnt = 0;
        }

        /* KEY1：短按光照上限 +1，长按 1s 后每 100ms -1 */
        if (KEY1 == 0)
        {
            key1_cnt++;
            if (key1_cnt == 2)
            {
                LOS_MuxPend(g_data_mux, LOS_WAIT_FOREVER);
                g_light_limit++;
                g_storage_changed = 1;
                LOS_MuxPost(g_data_mux);

                LED1 = !LED1;
                printf("LightLimit = %d %%\r\n", g_light_limit);
            }
            else if (key1_cnt == 50)
            {
                LOS_MuxPend(g_data_mux, LOS_WAIT_FOREVER);
                g_light_limit--;
                g_storage_changed = 1;
                LOS_MuxPost(g_data_mux);

                LED1 = !LED1;
                printf("LightLimit = %d %%\r\n", g_light_limit);
                key1_cnt = 45;
            }
        }
        else
        {
            key1_cnt = 0;
        }

        LOS_TaskDelay(20);      /* 20ms 采样周期 */
    }
}

/**************************************************************
* @brief  存储任务：检测到变化后写入 W25Q64
**************************************************************/
static void Task_Storage(void)
{
    int16_t temp_limit, light_limit;

    while (1)
    {
        LOS_TaskDelay(1000);    /* 每秒检查一次 */

        if (g_storage_changed)
        {
            LOS_MuxPend(g_data_mux, LOS_WAIT_FOREVER);
            temp_limit = g_temp_limit;
            light_limit = g_light_limit;
            LOS_MuxPost(g_data_mux);

            /* 原子写入：锁调度避免 Flash 擦写期间被抢占 */
            LOS_TaskLock();
            LimitStorage_Write(temp_limit, light_limit);
            LOS_TaskUnlock();

            LOS_MuxPend(g_data_mux, LOS_WAIT_FOREVER);
            g_storage_changed = 0;
            LOS_MuxPost(g_data_mux);
        }
    }
}

/**************************************************************
* @brief  传感器任务：ADC 采集 + NTC 公式转换为 °C 和 %
* @note   NTC 参数：R25=10K, B=3950, 分压电阻=10K（上臂）
*         电路：VCC(3.3V) → 10K → AO → NTC → GND
**************************************************************/
static void Task_Sensor(void)
{
    uint16_t adc_temp, adc_light;
    float vout, r_ntc, temp_k;
    uint16_t light_flipped;

    while (1)
    {
        /* 读取 ADC 原始值 */
        adc_temp  = AD_GetValue(ADC_CH_TEMP);    /* PA0: 热敏 */
        adc_light = AD_GetValue(ADC_CH_LIGHT);   /* PA5: 光敏 */

        /* 温度转换：NTC 10K B=3950 → 摄氏度 */
        vout   = adc_temp * 3.3f / 4095.0f;
        r_ntc  = 10000.0f * vout / (3.3f - vout);
        temp_k = 1.0f / (1.0f / 298.15f + logf(r_ntc / 10000.0f) / 3950.0f);

        /* 光照转换：翻转后映射到 0~100% */
        light_flipped = 4095 - adc_light;

        /* 更新共享数据 */
        LOS_MuxPend(g_data_mux, LOS_WAIT_FOREVER);
        g_temp  = (int16_t)(temp_k - 273.15f);
        g_light = (int16_t)(light_flipped * 100 / 4095);

        /* 实验四：光照 → 舵机角度 (0~100% → 0~180°) */
        g_servo_angle = (uint8_t)((uint32_t)g_light * 180 / 100);

        /* 实验四：温度 → 电机转速 (超上限时线性调速 0~100) */
        if (g_temp > g_temp_limit)
        {
            int16_t over = g_temp - g_temp_limit;
            g_motor_speed = over * 10;          /* 每超1°C +10转速 */
            if (g_motor_speed > 100) g_motor_speed = 100;
        }
        else
        {
            g_motor_speed = 0;
        }
        LOS_MuxPost(g_data_mux);

        /* 驱动舵机和电机 */
        Servo_SetAngle(g_servo_angle);
        Motor_SetSpeed(g_motor_speed);

        LOS_TaskDelay(500);     /* 每 500ms 采样一次 */
    }
}

/**************************************************************
* @brief  板级外设初始化
**************************************************************/
static void BSP_Init(void)
{
    MY_NVIC_PriorityGroupConfig(4);
    LED_Init();
    KEY_Init();
    uart_init(72, 115200);

    /* W25Q64 初始化（仅配置 GPIO/SPI，可在调度前调用） */
    LimitStorage_Init();

    /* 读取掉电保存的上下限，超出合理范围则重置 */
    LimitStorage_Read(&g_temp_limit, &g_light_limit);
    if (g_temp_limit < -40 || g_temp_limit > 125)  g_temp_limit = 35;
    if (g_light_limit < 0   || g_light_limit > 100) g_light_limit = 50;

    /* ADC 初始化（PA0 温度 + PA5 光照） */
    AD_Init();

    /* 实验四：舵机 + 电机初始化 */
    Servo_Init();
    Motor_Init();
}
