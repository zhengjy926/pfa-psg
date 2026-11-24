# MCU 外设与引脚配置

> 本文记录信号到芯片、外设、引脚复用、启动默认电平和中断的绑定。行为和验收见 [SRS](../srs/pulse-gen-firmware.md)。仍需确认的硬件配置见 [文档疑点](../ambiguities.md#硬件输入)。

外设设置与寄存器字段用于对照 CubeMX（LL/HAL）参考工程。

## 0 芯片与时钟

时钟链：HSE → ÷M → ×N → VCO → ÷R = SYSCLK；VCO ÷P 供 ADC1 / ADC2 时钟。

| 参数 | 值 | 单位 | 备注 |
| --- | --- | --- | --- |
| MCU 型号 | STM32G474QET6 | — | LQFP128 封装 |
| HSE 频率 | 8 | MHz | 晶振接 `PF0-OSC_IN` / `PF1-OSC_OUT`（§1） |
| PLL M | 2 | — | HSE ÷ M |
| PLL N | 85 | — | × N，得 VCO |
| PLL R | 2 | — | VCO ÷ R，得 SYSCLK |
| PLL P | 7 | — | VCO ÷ P，供 ADC 时钟支路 |
| VCO 频率 | 340 | MHz | 派生值 |
| SYSCLK / HCLK | 170 | MHz | 派生值 |
| ADC1 / ADC2 时钟 | 48.57 | MHz | 派生值。ADC1 / ADC2 同时采样上限为 52 MHz（DS12288 Table 66，Range 1 all ADCs single ended，VDDA≥2.7 V）。双通道采样沿用当前时钟。ASYNC DIV1 见 §2.1。 |
| 器件 Flash 容量 | 512 | KB | 固件使用量上限 256 KB（SRS N-002） |
| 器件 RAM 容量 | 128 | KB | 静态 RAM 使用量上限 64 KB（SRS N-002） |
| SysTick | 使能 | — | 抢占优先级见 §2.8 |
| Dead Battery | 禁用 | — | — |

### 0.1 Flash

STM32G474QET6 可用 `DBANK` 选项位配置双 Bank 模式。该模式下每个 Bank 为 256 KB，页大小为 2 KB，可一边从一个 Bank 取指或读取数据，一边擦写另一个 Bank。单页擦除时间上限为 24.47 ms。板上生效的 Bank 模式及存储布局见[硬件待补项](../ambiguities.md#硬件输入)。 [RM0440 §3.3.1](https://www2.st.com/resource/en/reference_manual/dm00355726-stm32g4-series-advanced-armbased-32bit-mcus-stmicroelectronics.pdf) · [STM32G474 数据手册 §3.4、表 47](https://www.st.com/resource/en/datasheet/stm32g474qe.pdf)

## 1 引脚分配

方向以 MCU 为参考。默认电平指 GPIO 初始化后的输出电平，复用输出的电平见外设配置。PA8、PA9 的 AF13 对应参考工程的 `LL_GPIO_AF_13`。需求 ID 查 [SRS](../srs/pulse-gen-firmware.md)，命令码查 [板卡命令总表](../spec/board-pulse-gen.md#31-命令总表)。

| 信号 | 引脚 | 方向 / AF | GPIO 默认电平 | 外设或配置 | 需求 | 命令 |
| --- | --- | --- | --- | --- | --- | --- |
| HSE 晶振输入 / 输出 | PF0 / PF1 | 输入 / 输出 | — | 时钟树 §0 | — | — |
| SWD 数据 / 时钟 | PA13 / PA14 | 复用 AF0 | — | SWDIO / SWCLK | I-027 | — |
| 正通道电流采样 | PA0 | 模拟 | — | ADC1_IN1；COMP3 INP | F-035、E-001 | 0x31、0x3C |
| 负通道电流采样 | PA1 | 模拟 | — | ADC2_IN2；COMP1 INP | F-035、E-001 | 0x31、0x3C |
| 正通道脉冲输出 | PA8 | 复用 AF13 | 见 §2.4 | HRTIM1_CHA1 | F-022 | 0x30、0x34 |
| 负通道脉冲输出 | PA9 | 复用 AF13 | 见 §2.4 | HRTIM1_CHA2 | F-022 | 0x30、0x34 |
| 端口启停输入 | PB4 | 复用（AF13） | — | HRTIM1_EEV7（引脚源、双边沿、滤波 0000）；输出复位 + CPT1 捕获 | F-003、F-012 | 0x30、0x3E；配置与门控见 §2.8 |
| R 波输入 | PB6 | 输入 | — | EXTI6，内部下拉 | F-006、F-010、F-031 | 0x32、0x36/0x37 |
| 脉冲引擎运行状态输出 | PB5 | 输出 | 低 | GPIO | F-014 | — |
| 板载 LED | PB9 | 输出 | 高 | GPIO | F-015 | — |
| 杆 A 驱动 | PD13 | 输出 | 低 | GPIO | F-027 至 F-034 | 0x40 至 0x43 |
| 杆 B 驱动 | PD12 | 输出 | 低 | GPIO | F-027 至 F-034 | 0x40 至 0x43 |
| 杆 C 驱动 | PD11 | 输出 | 低 | GPIO | F-027 至 F-034 | 0x40 至 0x43 |
| 杆 D 驱动 | PD10 | 输出 | 低 | GPIO | F-027 至 F-034 | 0x40 至 0x43 |
| 杆 E 驱动 | PD9 | 输出 | 低 | GPIO | F-027 至 F-034 | 0x40 至 0x43 |
| 杆 F 驱动 | PD8 | 输出 | 低 | GPIO | F-027 至 F-034 | 0x40 至 0x43 |
| 负通道过流锁存 | PG7 | 输入 | — | EXTI7，内部下拉 | E-001、E-003、E-010 | 0x31、0x3C、0x3D |
| 正通道过流锁存 | PG8 | 输入 | — | EXTI8，内部下拉 | E-001、E-003、E-010 | 0x31、0x3C、0x3D |
| 过流锁存复位 | PG9 | 输出 | 低 | GPIO；复位脉冲后等 5 ms 再重读锁存（E-003、I-021） | E-003、I-021 | 0x3D |
| 发布版协议口 TX / RX | PA2 / PA3 | 复用 AF7 | — | USART2_TX / USART2_RX | I-001、I-025、I-026 | 全命令集 |
| 单板调试版协议口 TX / RX | PC4 / PC5 | 复用 AF7 | — | USART1_TX / USART1_RX | I-001、I-025、I-026 | 全命令集 |
| 正、负通道比较器阈值 | 片内 | 模拟 | — | DAC3_OUT1 / DAC1_OUT1 | I-015、E-001 | 0x38/0x39 |

复位至 GPIO 初始化期间，除调试脚外 GPIO 为浮空输入，输出数据寄存器为 0。杆驱动 PD8 至 PD13 与锁存复位 PG9 在此期间由板级上下拉约束。初始化前的输出限制见 SRS E-006。

## 2 外设配置

引脚绑定见 §1。下面各节按外设给出连接、模式和寄存器取值。

### 2.1 ADC1 / ADC2

| 参数 | 值 | 单位 | 备注 |
| --- | --- | --- | --- |
| 采样通道 | ADC1：CH1（IN1）；ADC2：CH2（IN2） | — | — |
| 分辨率 | 12 | 位 | 两路一致 |
| 采样时间 | 12.5 | 周期 | ADC 采样阶段时长为 12.5 周期 ÷ 48.57 MHz ≈ 257 ns。ADC 引脚有效窗口和就绪时间见[采样验证](../design/current-settlement.md#截止时间与验证) |
| 外部触发源 | ADC1：HRTIM1 TRG1；ADC2：HRTIM1 TRG3 | — | 生成事件见 §2.4.4 |
| 时钟预分频 | ASYNC DIV1 | — | ADC 时钟为 48.57 MHz |
| 过采样 | 禁用 | — | — |
| DMA | 循环 | — | DMA 映射见 §2.7。缓冲固定最大尺寸（每路 2×256×2 B），HT/TC 仅数据就绪提示；串边界由 TIMA REP 串结束锚 + ISR 内 NDTR 快照推导，见[串级结算设计](../design/current-settlement.md#采样与判定) |

### 2.2 COMP1 / COMP3

| 参数 | 值 | 单位 | 备注 |
| --- | --- | --- | --- |
| 正输入 INP | COMP1：PA1（负通道）；COMP3：PA0（正通道） | — | 引脚见 §1；归属 F-035 |
| 负输入 INM | COMP1：`DAC1_OUT1`；COMP3：`DAC3_OUT1` | — | 内部 DAC 通道，见 §2.3 |
| 迟滞 | 70 | mV | 两路一致 |
| 触发模式 | 正向过渡 | — | 作用于比较器输出电平，术语见[正向过渡](../../GLOSSARY.md) |
| 过流检出输出 | COMP1 → FLT4；COMP3 → FLT5 | — | HRTIM 片内故障源，见 §2.4.3 |

### 2.3 DAC1 / DAC3

| 参数 | 值 | 单位 | 备注 |
| --- | --- | --- | --- |
| 输出通道 | DAC1：OUT1（内部）；DAC3：OUT1（内部） | — | 不出引脚；供比较器阈值 |
| 采样保持 | DAC1：禁用；DAC3：默认 | — | 取值见[待补项](../ambiguities.md#硬件输入) |
| 标定 | DAC1：出厂值；DAC3：默认 | — | 取值见[待补项](../ambiguities.md#硬件输入) |
| 用途 | DAC1：COMP1 阈值（负通道）；DAC3：COMP3 阈值（正通道） | — | 归属 I-015、E-001 |

### 2.4 HRTIM1

HRTIM 基准时钟 fHRTIM = SYSCLK = 170 MHz。定时器 A/B 使用 MUL2，计数时钟为 340 MHz，实际计数周期约 2.94 ns，标称量化分辨率 3 ns。时长按实际计数周期量化，规则见 [SRS T-009](../srs/pulse-gen-firmware.md#4-时序需求t-)。最小 PWM 约 5.19 kHz。突发模式使用 170 MHz 的 fHRTIM 经 512 分频。

#### 2.4.1 定时器 A（TA1 / TA2）

| 参数 | 值 | 单位 | 备注 |
| --- | --- | --- | --- |
| 输出 | TA1 → PA8；TA2 → PA9 | — | §1；归属 F-022 |
| 输出极性 | 两路均为高电平有效 | — | `HRTIM_OUTAR.POL1 = 0`、`POL2 = 0` |
| 模式 | 连续（CONTINUOUS） | — | — |
| 预分频 | MUL2（CKPSC = 100 → 计数时钟 = 2 × fHRTIM = 340 MHz） | — | RM0440 CKPSC 表 |
| 周期 | 65533（65534 计数 @340 MHz ≈ 192.7 µs） | 计数 | 实际周期计数为 PER + 1 |
| 比较单元 | CMP1 / CMP2 / CMP3 | — | 三个单元均参与置位与复位 |
| 比较值 | 65533 | 计数 | 上电占位。运行时由标定后的驱动时长和配置间隔按 T-009 换算为计数，再更新比较值 |
| 置位源 | TA1 ← CMP1；TA2 ← CMP3 | — | — |
| 复位源 | TA1 ← CMP2；TA2 ← PER | — | — |
| 空闲电平 | 两路均为低电平（INACTIVE） | — | `HRTIM_OUTAR.IDLES1 = 0`、`IDLES2 = 0` |
| 突发模式空闲控制 | 两路均响应突发控制器的空闲请求 | — | `HRTIM_OUTAR.IDLEM1 = 1`、`IDLEM2 = 1` |
| 重复计数 | 每串波形数 N 减 1，N 为 1–256 | — | TIMA REP 中断对应串结束，每串 1 次。更新使能（PREEN=1）。运行中写 REPR 于下一次更新事件转移，实际作用于下下串，按波形表提前一串预取写入（RM0440 §28.3.4、§28.5.18）。 |
| 预装载 | 禁用 | — | — |

软件进入 HRTIM 输出空闲态和突发控制器请求空闲时均应用上表的空闲电平。`INACTIVE` 按输出极性映射到引脚，故障输出配置见 §2.4.3。[ST RM0440 §28.3.14](https://www.st.com/resource/en/reference_manual/rm0440-stm32g4-series-advanced-armbased-32-bit-mcus-stmicroelectronics.pdf)

#### 2.4.2 定时器 B（TB，无输出）

| 参数 | 值 | 单位 | 备注 |
| --- | --- | --- | --- |
| 模式 | 连续（CONTINUOUS） | — | — |
| 预分频 | `MUL2` | — | 与定时器 A 一致 |
| 预装载 | 使能 | — | — |
| 更新触发 | Timer B 更新 | — | 寄存器字段见[待补项](../ambiguities.md#硬件输入) |
| 复位触发 | UPDATE | — | 寄存器字段见[待补项](../ambiguities.md#硬件输入) |

#### 2.4.3 故障与保护

| 参数 | 值 | 单位 | 备注 |
| --- | --- | --- | --- |
| 故障输入 | FLT4（负通道）/ FLT5（正通道） | — | 任一路过流均应使 TA1、TA2 进入故障输出态，见 SRS E-001、T-005 |
| 故障源 | FLT4 ← COMP1；FLT5 ← COMP3 | — | 选择片内比较器源，`FLT4SRC[1:0] = 01`、`FLT5SRC[1:0] = 01`。[ST RM0440 表 228](https://www.st.com/resource/en/reference_manual/dm00355726.pdf) |
| 故障输入极性 | 待核实 | — | 对照参考工程核实 `FLT4P`、`FLT5P`，见[硬件待补项](../ambiguities.md#硬件输入) |
| 滤波 | 无 | — | — |
| 复位模式 | 锁存 | — | 由条件复位改为锁存：过流消失后保持故障态停波，直至 0x3D 复位清除；台架重验 T-005 |
| 计数器值 | 2 | 计数 | — |
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
| 时钟源 | FHRTIM | — | 使用 170 MHz 的 HRTIM 基准时钟 |
| 预分频 | ÷512，突发时钟取 fHRTIM 170 MHz ≈ 332 kHz | — | RM0440（fBRST 源自 fHRTIM） |
| 触发源 | Timer A 重复 | — | REP=N-1 时每 N 周期一次；REP=0 时每周期一次。N×T_PWM 小于突发周期时丢触发，须 TABM=1（RM0440 §28.3.15） |
| 群边界参数重载 | 软件预装载（或 burst DMA：BRSTDMA=10、UPDGAT=0010） | — | G474 突发控制器不写 CMP/PER/REP 寄存器（RM0440 §28.3.15、§28.3.23），跨群换参数由软件在群间隔内完成 |

#### 2.4.6 中断

| 参数 | 值 | 单位 | 备注 |
| --- | --- | --- | --- |
| SYSFLT / FLT4 / FLT5 | `HRTIM1_FLT_IRQn` | — | FLT4 对应负通道，FLT5 对应正通道。优先级见 §2.8 |
| TIMA 重复（REP） | `HRTIM1_TIMA_IRQn` | — | 串结束事件；同线承载 CPT1 端口启停捕获（EEV7），优先级见 §2.8 |

### 2.5 IWDG

| 参数 | 值 | 单位 | 备注 |
| --- | --- | --- | --- |
| 预分频 | /8 | — | — |
| 重载 | 3999 | 计数 | — |
| 超时 | 标称 1，硬件约 0.941～1.085 | s | 按预分频、重载值及 LSI 频率边界计算。[STM32G474 数据手册表 45、85](https://www.st.com/resource/en/datasheet/stm32g474qe.pdf) |
| 计数时钟 | 标称 32，硬件 29.5～34 | kHz | 超时的基准。[STM32G474 数据手册表 45](https://www.st.com/resource/en/datasheet/stm32g474qe.pdf) |

### 2.6 USART1 / USART2

#### 2.6.1 通道配置

两路均使用异步收发，TX/RX 与复用见 §1。编译期通过 `CONFIG_PSG_PROTO_UART` 选择协议口，发布版默认用 `USART2`，单板调试版用 `USART1`，见 SRS I-025。

#### 2.6.2 参数配置（两路一致）

| 参数 | 值 | 单位 | 备注 |
| --- | --- | --- | --- |
| 波特率 | 115200 | bps | 与协议物理层同源 |
| 字长 | 8 | 位 | 字长包含校验位 |
| 校验 | 无 | — | — |
| 停止位 | 1 | 位 | — |
| 数据方向 | 收发 | — | — |
| 过采样 | 16 | — | — |
| 单采样 | 禁用 | — | — |
| 时钟预分频 | 1 | — | — |
| FIFO 模式 | 使能 | — | — |
| TX / RX FIFO 阈值 | 1/8 | — | — |
| 自动波特率 | 禁用 | — | — |
| TX / RX 引脚反相 | 禁用 | — | — |
| 数据反相 | 禁用 | — | — |
| TX / RX 引脚交换 | 禁用 | — | — |
| 溢出 | 使能 | — | 检测与恢复见[待补项](../ambiguities.md#硬件输入) |
| DMA on RX Error | 启用 | — | — |
| MSB First | 禁用 | — | — |

### 2.7 DMA

请求到 DMA 通道的唯一映射如下。所有请求的优先级为 Low。

| 请求 | DMA 通道 | 方向 | 模式 | 数据宽度 |
| --- | --- | --- | --- | --- |
| USART1_RX / USART2_RX | `DMA1_Channel1` / `DMA1_Channel3` | 外设→内存 | 普通 | 8 位 |
| USART1_TX / USART2_TX | `DMA1_Channel2` / `DMA1_Channel4` | 内存→外设 | 普通 | 8 位 |
| ADC1 / ADC2 | `DMA1_Channel5` / `DMA1_Channel6` | 外设→内存 | 循环 | 16 位 |

USART RX 表项为参考配置。PSG 优先验证循环 DMA 接收，并启用 USART 空闲（IDLE）中断。 [UART 接收设计](../design/uart-reception.md)

### 2.8 NVIC

优先级分组为 4 位抢占、0 位子优先级，对应 ST HAL 的 `NVIC_PRIORITYGROUP_4`。各中断的抢占优先级如下，0–3 段预留不用。

| 中断 | 抢占优先级 | 备注 |
| --- | --- | --- |
| HRTIM1_TIMA（CPT1：EEV7 端口启停捕获） | 4 | 串结束与端口事件同线，归属 F-003、F-012、F-038 |
| EXTI9_5（PB6 / PG7 / PG8） | 4 | PB6 / PG7 / PG8 对应 EXTI6 / EXTI7 / EXTI8 |
| HRTIM1_FLT | 4 | SYSFLT / FLT4 / FLT5，见 §2.4.6 |
| HRTIM1_TIMA | 4 | TIMA 重复事件，见 §2.4.6 |
| DMA1_Channel1–6 | 5 | — |
| USART1 | 5 | — |
| USART2 | 5 | — |
| SysTick | 15 | — |

HRTIM 保护类中断可抢占通信与 DMA 中断。同级中断彼此不抢占，同时挂起时按 IRQ 编号处理。比较器过流经 HRTIM 硬件故障态停波，ISR 采集故障来源与样本证据。PG7/PG8 用于观测独立过流锁存，实际关断及保持连线由 PSG-23 确认，须独立于 NVIC 调度。端口停止的 MCU 快路径采用硬件联动。PB4 使用 AF13=HRTIM1_EEV7，取消 EXTI4。EEV7 采用引脚源、双边沿、滤波 0000（RM0440 §28.3.8、§28.5.69），经 EXTEVNT7 直接复位两路输出（HRTIM_RSTx1R/RSTx2R），停波延迟典型 56 ns、最大 66 ns（DS12288 Table 62，同步模式、滤波关闭、fHRTIM = 170 MHz），不依赖中断；端口停止（负向过渡）与端口启动（正向过渡）由 EEV7→TIMA CPT1 捕获，经 HRTIM1_TIMA_IRQn 入队（ISR 读 PB4 电平判向），事件捕捉延迟不影响停波。启停输入源为协议（0x3E）时屏蔽 EEV7 输出复位与捕获。比较器故障通路（FLT4/5←COMP1/3）与 EEV7 使用不同输入。EEV7 后续脉冲持续封锁由 PSG-22 确认。配置 configMAX_SYSCALL_INTERRUPT_PRIORITY = 4，保护类 ISR 可调用 FromISR API。验收见 SRS F-012、E-001、T-005。[ST PM0214 §2.3.5–§2.3.6](https://www.st.com/resource/en/programming_manual/pm0214-stm32-cortexm4-mcus-and-mpus-programming-manual-stmicroelectronics.pdf)
