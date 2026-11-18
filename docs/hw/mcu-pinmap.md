# MCU 外设与引脚配置

> 本文记录信号到芯片、外设、引脚复用、启动默认电平和中断的绑定。行为和验收见 [SRS](../srs/pulse-gen-firmware.md)，量值见 [参数登记表](../params.md#7xx-mcu)。仍需确认的硬件配置见 [文档疑点](../ambiguities.md#硬件输入)。

外设设置与寄存器字段用于对照 CubeMX（LL/HAL）参考工程。

## 0 芯片与时钟

时钟链：HSE → ÷M → ×N → VCO → ÷R = SYSCLK；VCO ÷P 供 ADC1 / ADC2 时钟。

| 参数 | 值 | 单位 | 备注 |
| --- | --- | --- | --- |
| MCU 型号 | STM32G474QET6 | — | LQFP128 封装 |
| HSE 频率 | PRM-701 | MHz | 晶振接 `PF0-OSC_IN` / `PF1-OSC_OUT`（§1） |
| PLL M | PRM-702 | — | HSE ÷ M |
| PLL N | PRM-703 | — | × N，得 VCO |
| PLL R | PRM-704 | — | VCO ÷ R，得 SYSCLK |
| PLL P | PRM-705 | — | VCO ÷ P，供 ADC 时钟支路 |
| VCO 频率 | PRM-706 | MHz | 派生值 |
| SYSCLK / HCLK | PRM-707 | MHz | 派生值 |
| ADC1 / ADC2 时钟 | PRM-708 | MHz | 派生值；ASYNC DIV1 见 §2.1 |
| SysTick | 使能 | — | 抢占优先级见 §2.8 |
| Dead Battery | 禁用 | — | — |

## 1 引脚分配

方向以 MCU 为参考。默认电平指 GPIO 初始化后的输出电平，复用输出的电平见外设配置。PA8、PA9 的 AF13 对应参考工程的 `LL_GPIO_AF_13`。需求 ID 查 [SRS](../srs/pulse-gen-firmware.md)，命令码查 [板卡命令总表](../spec/board-pulse-gen.md#31-命令总表)。

| 信号 | 引脚 | 方向 / AF | GPIO 默认电平 | 外设或配置 | 需求 | 命令 |
| --- | --- | --- | --- | --- | --- | --- |
| HSE 晶振输入 / 输出 | PF0 / PF1 | 输入 / 输出 | — | 时钟树 §0 | — | — |
| SWD 数据 / 时钟 | PA13 / PA14 | 复用 AF0 | — | SWDIO / SWCLK | I-029 | — |
| 正通道电流采样 | PA0 | 模拟 | — | ADC1_IN1；COMP3 INP | F-035、E-001 | 0x31、0x3C |
| 负通道电流采样 | PA1 | 模拟 | — | ADC2_IN2；COMP1 INP | F-035、E-001 | 0x31、0x3C |
| 正通道脉冲输出 | PA8 | 复用 AF13 | 见 §2.4 | HRTIM1_CHA1 | F-022 | 0x30、0x34 |
| 负通道脉冲输出 | PA9 | 复用 AF13 | 见 §2.4 | HRTIM1_CHA2 | F-022 | 0x30、0x34 |
| 端口启停输入 | PB4 | 输入 | — | EXTI4，双边沿，内部下拉 | F-003、F-012 | 0x30、0x3E |
| R 波输入 | PB6 | 输入 | — | EXTI6，内部下拉 | F-006、F-009、F-031 | 0x32、0x36/0x37 |
| 运行指示 | PB5 | 输出 | 低 | GPIO | F-014 | — |
| 板载 LED | PB9 | 输出 | 高 | GPIO | F-015 | — |
| 杆 A 驱动 | PD13 | 输出 | 低 | GPIO | F-027 至 F-034 | 0x40 至 0x43 |
| 杆 B 驱动 | PD12 | 输出 | 低 | GPIO | F-027 至 F-034 | 0x40 至 0x43 |
| 杆 C 驱动 | PD11 | 输出 | 低 | GPIO | F-027 至 F-034 | 0x40 至 0x43 |
| 杆 D 驱动 | PD10 | 输出 | 低 | GPIO | F-027 至 F-034 | 0x40 至 0x43 |
| 杆 E 驱动 | PD9 | 输出 | 低 | GPIO | F-027 至 F-034 | 0x40 至 0x43 |
| 杆 F 驱动 | PD8 | 输出 | 低 | GPIO | F-027 至 F-034 | 0x40 至 0x43 |
| 负通道过流锁存 | PG7 | 输入 | — | EXTI7，内部下拉 | E-001、E-003、E-012 | 0x31、0x3C、0x3D |
| 正通道过流锁存 | PG8 | 输入 | — | EXTI8，内部下拉 | E-001、E-003、E-012 | 0x31、0x3C、0x3D |
| 过流锁存复位 | PG9 | 输出 | 低 | GPIO | E-003、I-021 | 0x3D |
| 发布版协议口 TX / RX | PA2 / PA3 | 复用 AF7 | — | USART2_TX / USART2_RX | I-001、I-027、I-028 | 全命令集 |
| 单板调试版协议口 TX / RX | PC4 / PC5 | 复用 AF7 | — | USART1_TX / USART1_RX | I-001、I-027、I-028 | 全命令集 |
| 正、负通道比较器阈值 | 片内 | 模拟 | — | DAC3_OUT1 / DAC1_OUT1 | I-015、E-001 | 0x38/0x39 |

复位至 GPIO 初始化期间，除调试脚外 GPIO 为浮空输入，输出数据寄存器为 0。杆驱动 PD8 至 PD13 与锁存复位 PG9 在此期间由板级上下拉约束。初始化前的输出限制见 SRS E-006。

## 2 外设配置

引脚绑定见 §1。下面各节按外设给出连接、模式和寄存器取值，登记量值按 [参数表](../params.md) 中的 PRM 引用读取。

### 2.1 ADC1 / ADC2

| 参数 | 值 | 单位 | 备注 |
| --- | --- | --- | --- |
| 采样通道 | PRM-734 | — | — |
| 分辨率 | PRM-709 | 位 | 两路一致 |
| 采样时间 | PRM-710 | 周期 | 采样窗口为 PRM-710 ÷ PRM-708，参见[台架待验证项](../ambiguities.md#psg-行为与验收) |
| 外部触发源 | ADC1：HRTIM1 TRG1；ADC2：HRTIM1 TRG3 | — | 生成事件见 §2.4.4 |
| 时钟预分频 | ASYNC DIV1 | — | 与 PRM-708 的 ADC 时钟对应 |
| 过采样 | PRM-735 | — | — |
| DMA | 循环 | — | DMA 映射见 §2.7 |

### 2.2 COMP1 / COMP3

| 参数 | 值 | 单位 | 备注 |
| --- | --- | --- | --- |
| 正输入 INP | COMP1：PA1（负通道）；COMP3：PA0（正通道） | — | 引脚见 §1；归属 F-035 |
| 负输入 INM | COMP1：`DAC1_OUT1`；COMP3：`DAC3_OUT1` | — | 内部 DAC 通道，见 §2.3 |
| 迟滞 | PRM-712 | mV | 两路一致 |
| 触发模式 | PRM-736 | — | 作用于比较器输出电平的正向过渡 |

### 2.3 DAC1 / DAC3

| 参数 | 值 | 单位 | 备注 |
| --- | --- | --- | --- |
| 输出通道 | DAC1：OUT1（内部）；DAC3：OUT1（内部） | — | 不出引脚；供比较器阈值 |
| 采样保持 | DAC1：禁用；DAC3：默认 | — | 取值见[待补项](../ambiguities.md#硬件输入) |
| 标定 | DAC1：出厂值；DAC3：默认 | — | 取值见[待补项](../ambiguities.md#硬件输入) |
| 用途 | DAC1：COMP1 阈值（负通道）；DAC3：COMP3 阈值（正通道） | — | 归属 I-015、E-001 |

### 2.4 HRTIM1

HRTIM 基准时钟 fHRTIM = SYSCLK（PRM-707）。定时器 A/B 计数时钟按 PRM-713 分频配置换算，计数分辨率约 2.94 ns，最小 PWM 约 5.19 kHz。突发模式直接使用 fHRTIM，按 PRM-718 分频。

#### 2.4.1 定时器 A（TA1 / TA2）

| 参数 | 值 | 单位 | 备注 |
| --- | --- | --- | --- |
| 输出 | TA1 → PA8；TA2 → PA9 | — | §1；归属 F-022 |
| 输出极性 | 两路均为高电平有效 | — | `HRTIM_OUTAR.POL1 = 0`、`POL2 = 0` |
| 模式 | 连续（CONTINUOUS） | — | — |
| 预分频 | PRM-713 | — | `MUL2` |
| 周期 | PRM-714 | 计数 | 实际周期计数为 PER + 1，见 PRM-714 |
| 比较单元 | CMP1 / CMP2 / CMP3 | — | 三个单元均参与置位与复位 |
| 比较值 | PRM-715 | 计数 | 上电占位。实际持续时间在运行时通过比较值更新，PRM-307、PRM-308 的非零范围约对应 68–1701 个计数 |
| 置位源 | TA1 ← CMP1；TA2 ← CMP3 | — | — |
| 复位源 | TA1 ← CMP2；TA2 ← PER | — | — |
| 空闲电平 | 两路均为低电平（INACTIVE） | — | `HRTIM_OUTAR.IDLES1 = 0`、`IDLES2 = 0` |
| 突发模式空闲控制 | 两路均响应突发控制器的空闲请求 | — | `HRTIM_OUTAR.IDLEM1 = 1`、`IDLEM2 = 1` |
| 重复计数 | PRM-716 | — | 更新禁用 |
| 预装载 | 禁用 | — | — |

软件进入 HRTIM 输出空闲态和突发控制器请求空闲时均应用上表的空闲电平。`INACTIVE` 按输出极性映射到引脚，故障输出配置见 §2.4.3。[ST RM0440 §28.3.14](https://www.st.com/resource/en/reference_manual/rm0440-stm32g4-series-advanced-armbased-32-bit-mcus-stmicroelectronics.pdf)

#### 2.4.2 定时器 B（TB，无输出）

| 参数 | 值 | 单位 | 备注 |
| --- | --- | --- | --- |
| 模式 | 连续（CONTINUOUS） | — | — |
| 预分频 | PRM-713 | — | `MUL2` |
| 预装载 | 使能 | — | — |
| 更新触发 | Timer B 更新 | — | 寄存器字段见[待补项](../ambiguities.md#硬件输入) |
| 复位触发 | UPDATE | — | 寄存器字段见[待补项](../ambiguities.md#硬件输入) |

#### 2.4.3 故障与保护

| 参数 | 值 | 单位 | 备注 |
| --- | --- | --- | --- |
| 故障输入 | FLT4 / FLT5 | — | 两路配置相同 |
| 故障源 | FLT4：`HRTIM_FAULT_4`；FLT5：`HRTIM_FAULT_5` | — | 信号绑定见[待补项](../ambiguities.md#硬件输入) |
| 滤波 | 无 | — | — |
| 复位模式 | 条件复位 | — | — |
| 计数器值 | PRM-717 | 计数 | — |
| 故障输出电平 | 两路均为低电平（INACTIVE） | — | `HRTIM_OUTAR.FAULT1[1:0] = 0b10`、`FAULT2[1:0] = 0b10` |

#### 2.4.4 ADC 触发

| 参数 | 值 | 单位 | 备注 |
| --- | --- | --- | --- |
| 触发输出 TRG1 | 源：Timer B CMP3；更新源：Timer B | — | ADC1 外部触发（§2.1） |
| 触发输出 TRG3 | 源：Timer B CMP4；更新源：Timer B | — | ADC2 外部触发（§2.1） |

#### 2.4.5 突发模式（Burst）

| 参数 | 值 | 单位 | 备注 |
| --- | --- | --- | --- |
| 使能 | 使能 | — | — |
| 模式 | 单次（SINGLESHOT） | — | — |
| 时钟源 | FHRTIM | — | 使用 HRTIM 基准 PRM-707 |
| 预分频 | PRM-718 | — | 分频取值及派生频率见 PRM-718 |
| 触发源 | Timer A 重复 | — | — |

#### 2.4.6 中断

| 参数 | 值 | 单位 | 备注 |
| --- | --- | --- | --- |
| SYSFLT / FLT4 / FLT5 | `HRTIM1_FLT_IRQn` | — | 优先级见 §2.8 |
| TIMA 重复（REP） | `HRTIM1_TIMA_IRQn` | — | 优先级见 §2.8 |

### 2.5 IWDG

| 参数 | 值 | 单位 | 备注 |
| --- | --- | --- | --- |
| 预分频 | PRM-719 | — | `/8` |
| 重载 | PRM-720 | 计数 | — |
| 超时 | PRM-603 | s | — |
| 计数时钟 | PRM-721 | kHz | LSI；超时的基准 |

### 2.6 USART1 / USART2

#### 2.6.1 通道配置

两路均使用异步收发，TX/RX 与复用见 §1。协议口选择见 PRM-737 与 SRS I-027。

#### 2.6.2 参数配置（两路一致）

| 参数 | 值 | 单位 | 备注 |
| --- | --- | --- | --- |
| 波特率 | PRM-002 | bps | 与协议物理层同源 |
| 字长 | PRM-003 | 位 | 字长包含校验位 |
| 校验 | PRM-005 | — | — |
| 停止位 | PRM-004 | 位 | — |
| 数据方向 | 收发 | — | — |
| 过采样 | PRM-722 | — | — |
| 单采样 | 禁用 | — | — |
| 时钟预分频 | PRM-723 | — | — |
| FIFO 模式 | 使能 | — | — |
| TX / RX FIFO 阈值 | PRM-724 | — | — |
| 自动波特率 | 禁用 | — | — |
| TX / RX 引脚反相 | 禁用 | — | — |
| 数据反相 | 禁用 | — | — |
| TX / RX 引脚交换 | 禁用 | — | — |
| 溢出 | 使能 | — | 检测与恢复见[待补项](../ambiguities.md#硬件输入) |
| DMA on RX Error | 启用 | — | — |
| MSB First | 禁用 | — | — |

### 2.7 DMA

请求到 DMA 通道的唯一映射见 PRM-738，所有请求的优先级见 PRM-726。

| 请求 | 方向 | 模式 | 数据宽度 |
| --- | --- | --- | --- |
| USART1_RX / USART2_RX | 外设→内存 | 普通 | PRM-739 |
| USART1_TX / USART2_TX | 内存→外设 | 普通 | PRM-739 |
| ADC1 / ADC2 | 外设→内存 | 循环 | PRM-725 |

### 2.8 NVIC

优先级分组见 PRM-727。各中断的抢占优先级分档如下，数值见 PRM-729 至 PRM-731。

| 中断 | 抢占优先级 | 备注 |
| --- | --- | --- |
| EXTI4（PB4 端口启停输入） | PRM-731 | 双边沿，归属 F-003、F-012 |
| EXTI9_5（PB6 / PG7 / PG8） | PRM-731 | PB6 / PG7 / PG8 对应 EXTI6 / EXTI7 / EXTI8 |
| HRTIM1_FLT | PRM-731 | SYSFLT / FLT4 / FLT5，见 §2.4.6 |
| HRTIM1_TIMA | PRM-731 | TIMA 重复事件，见 §2.4.6 |
| DMA1_Channel1–6 | PRM-730 | — |
| USART1 | PRM-730 | — |
| USART2 | PRM-730 | — |
| SysTick | PRM-729 | — |

EXTI4 可抢占通信与 DMA 中断。同级中断彼此不抢占，同时挂起时按 IRQ 编号处理。端口启停应走短且有界的中断路径，停波与同时过流的验收见 SRS F-012、E-001、T-005。硬件过流停波独立于 NVIC 调度。[ST PM0214 §2.3.5–§2.3.6](https://www.st.com/resource/en/programming_manual/pm0214-stm32-cortexm4-mcus-and-mpus-programming-manual-stmicroelectronics.pdf)
