# STM32 分布式新能源课程实验 - 项目记忆

> ⚠️ 本文件为 Agent 工作记忆，项目说明以根目录 **`AGENTS.md`** 为准。
> 本文件只记录未在 AGENTS.md 中体现的操作细节和中间决策。

## 项目概况
- 课程：分布式新能源课程，共 5 个 STM32 实验
- 实验一：LiteOS 移植（基于按键控制 LED）
- 实验二：按键 + OLED 显示 + W25Q64 存储
- 实验三：温度及光照检测（ADC）
- 实验四：舵机和直流电机控制（PWM）
- 实验五：综合实验

## 实验一关键技术信息
- 工程位置：`实验沙盒一/test_key1/`（已移植 LiteOS），主文件 `USER/test.c`，工程文件 `test.uvprojx`
- **硬件映射（C8T6 最小系统板，已适配）**：
  - DS0 = LED0 = PB5（led.h: `#define LED0 PBout(5)`）
  - DS1 = LED1 = PB0（led.h: `#define LED1 PBout(0)`）
  - KEY0 = PB1，KEY1 = PA1（key.h，PB2 在最小系统板上未引出）
  - 串口：uart_init(72,115200)，printf 可用；PA9(TX)、PA10(RX)
- **调试器**：DAP mini；SWDIO→PA13，SWCLK→PA14，3.3V，GND
- **串口模块**：USB-TTL；TXD→PA10，RXD→PA9，GND 共地
- LiteOS 移植已完成：arch/arm/arm-m + kernel + cmsis + OS_CONFIG
- ⚠️ LiteOS 下禁止使用裸机 delay_ms（Systick 被 LiteOS 接管），延时改用 LOS_TaskDelay
- test.c 已改写为实验一要求的按键控 LED + 串口打印：
  - 新增两个 LiteOS 任务 `Task_DS0` 和 `Task_DS1`，分别检测 KEY0 和 KEY1
  - 按键按下后翻转对应 LED（DS0/DS1），并串口打印按键状态
  - 使用 `LOS_TaskDelay(20)` 做消抖，不使用 `KEY_Scan`（避免 static 状态冲突）
- 驱动层 C8T6 适配改动：led.c 初始化 PB5/PB0；key.c 初始化 PB1/PB2 上拉输入
- **工程配置已改为 C8T6（Medium Density）**：
  - `USER/test.uvprojx` 编译宏由 `STM32F10X_HD` 改为 `STM32F10X_MD`
  - 启动文件由 `startup_stm32f10x_hd.s` 改为 `USER/startup_stm32f10x_md.s`
  - 烧录前建议在 Keil 中把 Device 从 STM32F103ZE 改为 STM32F103C8

## 另一个参考项目
- `实验沙盒一/3-4 按键控制LED/`：裸机例程，江协科技库（与 test_key1 的正点原子库不同，不可混用）
  - LED: PA1(LED1)/PA2(LED2)，KEY: PB1/PB11

## 实验二 实践教训
- 移植江协库驱动到正点原子工程时，必须创建 `stm32f10x_conf.h` 并取消注释 `USE_STDPERIPH_DRIVER`。
- W25Q64 Flash 写入期间必须用 `LOS_TaskLock/Unlock` 锁调度，`WaitBusy()` 用纯空循环——`LOS_TaskDelay` 会打断 I2C 导致 OLED 雪花。
- OLED 初始化前需 ≥100ms 空循环延时，不能放在任务中用 `LOS_TaskDelay`（调度尚未启动时无效）。

## 实验三 实践教训
- MH-Sensor-Series 传感器在分压下臂（VCC→10K→AO→传感器→GND），AO 电压与物理量反向。
- NTC 温度用 B 参数方程：T(K)=1/(1/T0+ln(R/R0)/B)，模块默认参数 R25=10K, B=3950。
- OLED 偏移修复需同时重置 0x40（起始行）和 0xD3（偏移）两个寄存器。
- ADC 通道选择需避开 PA1~PA4（被 W25Q64 软件 SPI 占用），可用 PA0、PA5~PA7。

## 实验四 实践教训
- 参考例程（江协库）默认引脚与正点原子工程冲突严重，必须重新分配 PWM 引脚。
- 舵机用 TIM3_CH1（PA6）+ 电机用 TIM2_CH3（PB10），两路 PWM 用不同定时器避免周期冲突。
- 电机驱动模块最终确认为 **TB6612FNG**（不是 L298N），模块端口布局：
  - 左排：PWMA, AIN2, AIN1, STBY, BIN1, BIN2, PWMB, GND
  - 右排：VM, VCC, GND, AO1, AO2, BO1, BO2, GND
  - 本实验使用 A 通道：AIN1/AIN2 控制方向，PWMA 接 PWM 调速，STBY 必须接 3.3V 使能。
- TIM2_CH3 默认在 PA2，要输出到 PB10 必须用 `GPIO_PartialRemap2_TIM2`（不是 PartialRemap1），这是电机不转的关键 bug。
- TIM 库函数需在 test.uvprojx 中显式添加 stm32f10x_tim.c 到 FWlib 组。
- stm32f10x_conf.h 中需包含 stm32f10x_tim.h（此工程已有）。
- 实验四完全从实验三代码叠加，零代码删除，只新增驱动和逻辑行。
- OLED 四行布局：T/L 实时值 → TL/LL 上下限 → S/M 舵机角度/电机转速 → 保存状态。

## 实验五 实践教训
- 在实验四工程基础上直接扩展，保留全部实验四代码和驱动，新增 `Task_Comm` 串口任务。
- 实验五核心需求：温度/光照分别与保存的上下限比较，**超过才动作**；电机转速/舵机角度与超出值成正比。
- 光照上限存储和显示均使用 0~100（%），实验四遗留的 `DEFAULT_LIGHT_LIMIT=500` 和校验范围 0~4095 必须修正为 0~100。
- 串口命令协议：`T<值>` / `L<值>`（回车换行结尾），PC 可设置温度/光照上限；下位机每 500ms 自动上传数据帧。
- 串口接收使用 `USART_RX_STA` 标志位（bit15=完成），解析后必须清零，否则无法接收下一条命令。
- 上位机与下位机通信约定：115200 8N1，帧格式 `T:xxC L:xx% TL:xxC LL:xx% S:xxx M:xxx`。

