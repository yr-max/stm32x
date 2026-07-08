#include "stm32f10x.h"
#include "los_sys.h"
#include "los_typedef.h"
#include "los_task.ph"

#include "sys.h"
#include "led.h"
#include "usart.h"
#include "key.h"

/* 任务 ID 句柄 */
UINT32 Task_DS0_Handle;
UINT32 Task_DS1_Handle;

/* 函数声明 */
static UINT32 AppTaskCreate(void);
static UINT32 Create_DS0_Task(void);
static UINT32 Create_DS1_Task(void);
static void Task_DS0(void);
static void Task_DS1(void);
static void BSP_Init(void);

/***************************************************************
* @brief 主函数
* @note  第一步：板载硬件初始化
*        第二步：创建 LiteOS 应用任务
*        第三步：启动 LiteOS 多任务调度
**************************************************************/
int main(void)
{
    UINT32 uwRet = LOS_OK;

    Stm32_Clock_Init(9);    // 系统时钟设置 72MHz
    BSP_Init();

    printf("LiteOS Key-LED Demo Start\r\n");

    /* LiteOS 内核初始化 */
    uwRet = LOS_KernelInit();
    if (uwRet != LOS_OK)
    {
        printf("LiteOS 核心初始化失败！失败代码 0x%X\r\n", uwRet);
        return LOS_NOK;
    }

    /* 创建 LiteOS 任务 */
    uwRet = AppTaskCreate();
    if (uwRet != LOS_OK)
    {
        printf("AppTaskCreate 创建任务失败！失败代码 0x%X\r\n", uwRet);
        return LOS_NOK;
    }

    /* 开启 LiteOS 任务调度 */
    LOS_Start();

    /* 正常不会执行到这里 */
    while (1);
}

/**************************************************************
* @brief  应用任务创建：集中管理所有任务的创建
**************************************************************/
static UINT32 AppTaskCreate(void)
{
    UINT32 uwRet = LOS_OK;

    /* 创建 KEY0 -> DS0 任务 */
    uwRet = Create_DS0_Task();
    if (uwRet != LOS_OK)
    {
        printf("DS0 任务创建失败！失败代码 0x%X\r\n", uwRet);
        return uwRet;
    }

    /* 创建 KEY1 -> DS1 任务 */
    uwRet = Create_DS1_Task();
    if (uwRet != LOS_OK)
    {
        printf("DS1 任务创建失败！失败代码 0x%X\r\n", uwRet);
        return uwRet;
    }

    return LOS_OK;
}

/**************************************************************
* @brief  创建 DS0 任务：KEY0 控制 DS0 亮灭
**************************************************************/
static UINT32 Create_DS0_Task(void)
{
    UINT32 uwRet = LOS_OK;
    TSK_INIT_PARAM_S task_init_param;

    task_init_param.usTaskPrio = 5;                         /* 优先级 5 */
    task_init_param.pcName = "Task_DS0";                    /* 任务名 */
    task_init_param.pfnTaskEntry = (TSK_ENTRY_FUNC)Task_DS0; /* 入口函数 */
    task_init_param.uwStackSize = 1024;                     /* 栈大小 1024 字节 */

    uwRet = LOS_TaskCreate(&Task_DS0_Handle, &task_init_param);
    return uwRet;
}

/**************************************************************
* @brief  创建 DS1 任务：KEY1 控制 DS1 亮灭
**************************************************************/
static UINT32 Create_DS1_Task(void)
{
    UINT32 uwRet = LOS_OK;
    TSK_INIT_PARAM_S task_init_param;

    task_init_param.usTaskPrio = 5;                         /* 优先级 5 */
    task_init_param.pcName = "Task_DS1";                    /* 任务名 */
    task_init_param.pfnTaskEntry = (TSK_ENTRY_FUNC)Task_DS1; /* 入口函数 */
    task_init_param.uwStackSize = 1024;                     /* 栈大小 1024 字节 */

    uwRet = LOS_TaskCreate(&Task_DS1_Handle, &task_init_param);
    return uwRet;
}

/**************************************************************
* @brief  DS0 任务实现：KEY0 按下 -> DS0 翻转
* @note   KEY0 为低电平有效，LED 也是低电平有效（0=亮，1=灭）
**************************************************************/
static void Task_DS0(void)
{
    while (1)
    {
        if (KEY0 == 0)                  /* KEY0 被按下 */
        {
            LOS_TaskDelay(20);          /* 消抖延时 20ms */
            if (KEY0 == 0)              /* 确认确实按下 */
            {
                LED0 = !LED0;           /* 翻转 DS0 */
                printf("KEY0 pressed, DS0 is %s\r\n", LED0 ? "OFF" : "ON");

                while (KEY0 == 0)       /* 等待按键释放 */
                {
                    LOS_TaskDelay(20);  /* 让出 CPU，避免死等 */
                }
                LOS_TaskDelay(20);      /* 释放消抖 */
            }
        }
        LOS_TaskDelay(20);              /* 轮询间隔 */
    }
}

/**************************************************************
* @brief  DS1 任务实现：KEY1 按下 -> DS1 翻转
* @note   KEY1 为低电平有效，LED 也是低电平有效（0=亮，1=灭）
**************************************************************/
static void Task_DS1(void)
{
    while (1)
    {
        if (KEY1 == 0)                  /* KEY1 被按下 */
        {
            LOS_TaskDelay(20);          /* 消抖延时 20ms */
            if (KEY1 == 0)              /* 确认确实按下 */
            {
                LED1 = !LED1;           /* 翻转 DS1 */
                printf("KEY1 pressed, DS1 is %s\r\n", LED1 ? "OFF" : "ON");

                while (KEY1 == 0)       /* 等待按键释放 */
                {
                    LOS_TaskDelay(20);  /* 让出 CPU，避免死等 */
                }
                LOS_TaskDelay(20);      /* 释放消抖 */
            }
        }
        LOS_TaskDelay(20);              /* 轮询间隔 */
    }
}

/**************************************************************
* @brief  板级外设初始化
**************************************************************/
static void BSP_Init(void)
{
    MY_NVIC_PriorityGroupConfig(4);     /* 中断优先级分组：4 位抢占优先级 */
    LED_Init();                         /* 初始化 DS0/DS1 */
    KEY_Init();                         /* 初始化 KEY0/KEY1 */
    uart_init(72, 115200);              /* 串口初始化 115200 */
}
