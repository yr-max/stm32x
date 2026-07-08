# 分布式新能源课程 — STM32 实验项目规范

> Agent 每次打开自动加载。详细内容参见 `实验报告/refs/`。

---

## 1. 编译环境

| 项目 | 值 |
|------|-----|
| IDE | Keil MDK5 |
| 路径 | `D:\Keil_v5\UV4\UV4.exe` |
| 编译器 | ARMCC V5.06 |
| 编译命令 | `"D:\Keil_v5\UV4\UV4.exe" -b <工程路径>\test.uvprojx -o <输出日志>` |
| 工程入口 | `实验沙盒五/test_key1/USER/test.uvprojx`（实验五，当前最新） |

---

## 2. 硬件平台

| 项目 | 规格 |
|------|------|
| 芯片 | STM32F103C8T6（Medium Density） |
| 调试器 | DAP mini（SWD + 串口） |
| 编译宏 | `STM32F10X_MD` |
| 启动文件 | `startup_stm32f10x_md.s` |

> 完整物料清单：`实验报告/refs/硬件物料清单.md`


---

## 3. 实验进度

| # | 名称 | 工程位置 | 状态 |
|---|------|---------|------|
| 一 | LiteOS 移植 | `实验沙盒一/test_key1/` | ✅ |
| 二 | 按键与显示 | `实验沙盒二/test_key1/` | ✅ |
| 三 | 温度及光照检测 | `实验沙盒三/test_key1/` | ✅ 完成 |
| 四 | 舵机和直流电机控制 | `沙盒实验四/test_key1/` | ✅ 完成 |
| 五 | 综合实验 | `实验沙盒五/test_key1/` | ✅ 完成 |

---

## 4. 全局核心约定

- ⚠️ **LiteOS 下禁止 `delay_ms()`**（SysTick 被 LiteOS 接管），改用 `LOS_TaskDelay()`。
- 按键读取用位带宏（`KEY0`/`KEY1`），不用 `KEY_Scan()`。
- LED 低电平点亮：`LED0 = 0` 亮，`LED0 = 1` 灭。
- 串口 115200，`printf()` 可用。
- ⚠️ **W25Q64 写入时用 `LOS_TaskLock/Unlock` 锁调度**，`WaitBusy()` 使用纯空循环。
- 移植江协库驱动到正点原子工程需：创建 `stm32f10x_conf.h` + 取消注释 `USE_STDPERIPH_DRIVER` + 添加 FWlib 源文件到工程。

---

## 5. 实验一概要

> 详见 `实验报告/refs/实验一_代码说明.md`

- 移植 LiteOS，主逻辑 `USER/test.c` 双任务按键控 LED + 串口。
- 原引脚 LED PB5/PB0、KEY PB1/PA1。

---

## 6. 实验二概要

> 详见 `实验报告/refs/实验二_代码说明.md` · `实验报告/refs/实验二_硬件映射.md`

- 4 个 LiteOS 任务：Display / Key / Storage / Sensor。
- OLED 显示 T/L（模拟数据）和 TL/LL（按键调，W25Q64 掉电保存）。
- W25Q64 软件 SPI 引脚 PA4/PA3/PA2/PA1。
- 任务要求：`实验要求.md` 实验二章节。

---

## 7. 实验三概要

> 详见 `实验报告/refs/实验三_硬件映射.md`

- 5 个 LiteOS 任务：Display / Key / Storage / Sensor（ADC 采样）。
- 热敏模块 PA0 → NTC 10K B=3950 公式转摄氏度（°C）。
- 光敏模块 PA5 → 线性映射 0~100%。
- OLED 显示 `T:25C L:50%` / `TL:35C LL:50%`。
- ⚠️ OLED 偏移修复：`OLED_ResetOffset()` 每 200ms 重置 0x40+0xD3。
- 任务要求：`实验要求.md` 实验三章节。

---

## 8. 实验四概要

> 详见 `实验报告/refs/实验四_硬件映射.md` · `实验报告/refs/实验四_代码说明.md`

- 在实验三基础上扩展，新增舵机 + 直流电机 PWM 控制。
- 5 个 LiteOS 任务不变，Sensor 任务中新增 PWM 控制逻辑。
- **舵机**：TIM3_CH1 → PA6，50Hz（20ms），光照 L% 映射到角度 0~180°。
- **直流电机**：TIM2_CH3 → PB10（PWM）+ PB8/PB9（TB6612FNG 方向），温度超标时线性调速。
- OLED 显示 `S:90 M:50`（舵机角度 / 电机转速）。
- 任务要求：`实验要求.md` 实验四章节。

### 8.1 实验四新增引脚

| 功能 | 引脚 | 定时器 | 说明 |
|------|------|--------|------|
| 舵机 PWM | PA6 | TIM3_CH1 | 50Hz, 0~180° |
| 电机 PWM | PB10 | TIM2_CH3 | 10kHz |
| 电机 IN1 | PB8 | GPIO | TB6612FNG 方向控制 |
| 电机 IN2 | PB9 | GPIO | TB6612FNG 方向控制 |

> 避开 PA1~PA4（W25Q64 软件 SPI），两路 PWM 用不同定时器互不干扰。
> TB6612FNG 端口：左排 `PWMA,AIN2,AIN1,STBY,BIN1,BIN2,PWMB,GND`；右排 `VM,VCC,GND,AO1,AO2,BO1,BO2,GND`。
> 本实验使用 A 通道（AIN1/AIN2/PWMA），B 通道悬空。

---

## 9. 实验五概要

> 详见 `实验报告/refs/实验五_代码说明.md` · `实验报告/refs/实验五_硬件映射.md`

- 在实验四基础上扩展为综合实验：温度/光照采集 + OLED 显示 + 串口上传 + 舵机/电机控制。
- 5 个 LiteOS 任务：Display / Key / Storage / Sensor / Comm。
- 温度超过上限 → 电机转速与超出值成正比（0~100）。
- 光照超过上限 → 舵机角度与超出值成正比（0~180°）。
- PC 可通过串口命令 `T<值>` / `L<值>` 设置温度/光照上限（例：T35、L50）。
- 串口每 500ms 自动上传：`T:xxC L:xx% TL:xxC LL:xx% S:xxx M:xxx`。
- 任务要求：`实验要求.md` 实验五章节。

---

## 10. 目录结构

```
E:\aiandlearning\stm32x\
├── AGENTS.md                        ← 本文件
├── 实验要求.md
├── 实验报告/
│   └── refs/                        ← 硬件映射、代码说明、物料清单
├── 实验沙盒一/                      ← 实验一
│   └── test_key1/                   ← LiteOS 移植版
├── 实验沙盒二/                      ← 实验二
│   ├── 3-4 按键控制LED/             ← 参考例程
│   ├── 4-1 OLED显示屏/
│   ├── 11-1 软件SPI读写W25Q64/
│   ├── 11-2 硬件SPI读写W25Q64/
│   └── test_key1/                   ← 实验二工程
├── 实验沙盒三/                      ← 实验三
│   ├── 7-2 AD多通道/                ← 参考例程
│   ├── 8-2 DMA+AD多通道/            ← 参考例程
│   └── test_key1/                   ← 实验三工程
├── 沙盒实验四/                      ← 实验四
│   ├── 6-4 PWM驱动舵机/             ← 参考例程
│   ├── 6-5 PWM驱动直流电机/         ← 参考例程
│   └── test_key1/                   ← 实验四工程
├── 实验沙盒五/                      ← 实验五（综合实验）
│   └── test_key1/                   ← 实验五工程
├── LiteOS/                           ← LiteOS 源码
├── 程序源码/、模块资料/、资料/
└── .workbuddy/                       ← Agent 工作记忆
```

---

## 11. 维护规范

1. **AGENTS.md** 保持精简，只放核心信息和路径引用。
2. 详细内容放在 `实验报告/refs/`，AGENTS.md 引用。
3. 每个实验完成后更新进度和引脚映射。
4. 引脚分配变化时同步更新 §2.1。
