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

/* 共享数据 */
int16_t g_temp = 25;           /* 当前温度（模拟数据，实验三接入 ADC） */
int16_t g_light = 800;         /* 当前光照（模拟数据，实验三接入 ADC） */
int16_t g_temp_limit = 35;     /* 温度上限 */
int16_t g_light_limit = 500;   /* 光照上限 */
uint8_t g_storage_changed = 0; /* 上下限变化标志，需要保存 */

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

    printf("LiteOS Exp2 Key-OLED-W25Q64 Start\r\n");

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
        LOS_MuxPend(g_data_mux, LOS_WAIT_FOREVER);

        sprintf(buf, "T:%-3d L:%-4d", g_temp, g_light);
        OLED_ShowString(1, 1, buf);

        sprintf(buf, "TL:%-3dLL:%-4d", g_temp_limit, g_light_limit);
        OLED_ShowString(2, 1, buf);

        if (g_storage_changed)
        {
            OLED_ShowString(3, 1, "Saving...    ");
        }
        else
        {
            OLED_ShowString(3, 1, "Saved OK     ");
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
                printf("TempLimit = %d\r\n", g_temp_limit);
            }
            else if (key0_cnt == 50)    /* 长按约 1s */
            {
                LOS_MuxPend(g_data_mux, LOS_WAIT_FOREVER);
                g_temp_limit--;
                g_storage_changed = 1;
                LOS_MuxPost(g_data_mux);

                LED0 = !LED0;
                printf("TempLimit = %d\r\n", g_temp_limit);
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
                printf("LightLimit = %d\r\n", g_light_limit);
            }
            else if (key1_cnt == 50)
            {
                LOS_MuxPend(g_data_mux, LOS_WAIT_FOREVER);
                g_light_limit--;
                g_storage_changed = 1;
                LOS_MuxPost(g_data_mux);

                LED1 = !LED1;
                printf("LightLimit = %d\r\n", g_light_limit);
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
* @brief  传感器任务：模拟温/光数据变化
* @note   实验三接入 ADC 后替换为真实采样
**************************************************************/
static void Task_Sensor(void)
{
    int16_t temp_dir = 1;
    int16_t light_dir = 1;

    while (1)
    {
        LOS_MuxPend(g_data_mux, LOS_WAIT_FOREVER);

        g_temp += temp_dir;
        if (g_temp >= 50) temp_dir = -1;
        if (g_temp <= 10) temp_dir = 1;

        g_light += light_dir * 10;
        if (g_light >= 1500) light_dir = -1;
        if (g_light <= 100) light_dir = 1;

        LOS_MuxPost(g_data_mux);

        LOS_TaskDelay(500);     /* 每 500ms 更新一次模拟数据 */
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

    /* 读取掉电保存的上下限 */
    LimitStorage_Read(&g_temp_limit, &g_light_limit);
}
