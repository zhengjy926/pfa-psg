# 文档疑点与待补项

按影响范围记录冲突、待补输入和待验证数据。确认后更新对应条款与参数，在本页保留结论出处；已有 GitHub Issue 时附编号。

## PSG 行为与验收

| ID | 已确认的验收项 | 待补数据 | 裁决 | 正式条款 |
| --- | --- | --- | --- | --- |
| PSG-01 | 按 [SRS F-035](srs/pulse-gen-firmware.md#24-电流采样齐样与结算) 和[串级结算设计](design/current-settlement.md#截止时间与验证) 验证最短非零脉冲、跨串首样和按串齐样。 | 提交标定后的实际功率输出、ADC 引脚有效窗口及采样误差、DMA 半区归属，以及最短串间隔下从采样到放行下一串的最坏用时和余量。 | 以台架测得的功率输出、ADC 引脚有效窗口和 DMA 就绪时刻验收最短非零配置。 | [SRS F-035](srs/pulse-gen-firmware.md#24-电流采样齐样与结算)、[采样验证](design/current-settlement.md#截止时间与验证) |
| PSG-02 | 按 [SRS F-040](srs/pulse-gen-firmware.md#24-电流采样齐样与结算) 共同分配群级等待与结算预算，并留出正余量。 | 提交最坏负载下的两项用时上界、总用时和相对时限的余量。 | 最坏负载下，群级等待与结算总预算相对 PRM-118 的边界和 PRM-306 的最小群间隔均留有正余量。 | [SRS F-040](srs/pulse-gen-firmware.md#24-电流采样齐样与结算)、[群级预算验证](design/current-settlement.md#截止时间与验证) |
| PSG-03 | 按 [SRS F-012](srs/pulse-gen-firmware.md#21-启停与触发) 和 [F-022](srs/pulse-gen-firmware.md#22-出波) 验证端口停波及输出电平，并发过流路径按 T-005 单独验收。 | 提交初始化后空闲及故障电平、事件对齐波形、并发过流归属和 PB4 负向过渡至两路停波的最坏延迟。PB4 停波验收阈值按实测最坏延迟与安全分析上限取严，上限待专项安全分析。 | 在通信负载下验证 PB4 停止与 R 波、定时事件和过流并发，保留过流归属并单独核对 T-005。 | [SRS F-012](srs/pulse-gen-firmware.md#21-启停与触发)、[F-022](srs/pulse-gen-firmware.md#22-出波) |
| PSG-12 | 按 [SRS T-005](srs/pulse-gen-firmware.md#4-时序需求t-) 分别验证比较器过流检出与独立过流锁存到双路停波的时限。 | 核对板级功率关断路径，提交保持原杆驱动状态时的实际功率输出和出波电流波形，以及两条通路的最坏停波延迟。 | Q15 确认取消过流保护触发的六杆全断。过流检出、过流锁存分别按 PRM-509、PRM-510 验收双路停波。 | [SRS F-028](srs/pulse-gen-firmware.md#23-杆与杆选通)、[E-001、E-005](srs/pulse-gen-firmware.md#5-异常与安全需求e-)、[T-005](srs/pulse-gen-firmware.md#4-时序需求t-) |

## 硬件输入

| ID | 疑点 | 需要输入 |
| --- | --- | --- |
| HW-01 | [DAC 配置](hw/mcu-pinmap.md#23-dac1--dac3) 使用「默认」「出厂值」。 | 提供生效配置、寄存器值或决定默认值的驱动与版本。 |
| HW-02 | [HRTIM Timer B](hw/mcu-pinmap.md#242-定时器-btb无输出) 的更新和复位只记录事件名。[故障与保护](hw/mcu-pinmap.md#243-故障与保护) 已确定 FLT4 来自 COMP1（负通道），FLT5 来自 COMP3（正通道），故障输入极性待核实。 | 提供 Timer B 更新、复位的寄存器配置及 HRTIM `FLT4P`、`FLT5P` 的参考工程配置或实机读值。 |
| HW-03 | [USART 配置](hw/mcu-pinmap.md#262-参数配置两路一致) 给出溢出使能和 RX 错误时启用 DMA，缺检测与恢复触发。 | 提供 ORE 与 DMA 错误处理、清除顺序和恢复接收行为。 |
| HW-04 | PSG 采用[双 Bank Flash 布局](adr/0002-dual-bank-flash-layout.md)，实板配置和擦写期间的时序待验证。 | 核对 Flash 选项字节和链接映射中的中断向量、代码、常量及非易失数据页位置。测量擦写期间的中断和前台调度延迟，以及 0x03、0x05 的完整应答用时。 |
| HW-05 | [UART 半包计时](design/uart-reception.md#半包计时与验收) 的实板计时精度待验证。 | 核对 DMA 进度采样记录与线路上的字节收齐时刻，测量最坏负载下的计时误差并确认台架验收误差带。 |

## 整机待补项

本工程范围为 PFA_PSG。下列编号对应延期到整机或其它板卡工程处理的联调输入。

| 编号 | 需要输入或核对 |
| --- | --- |
| A-02 | 确认显控板链路的物理层配置，以及所有链路两端串口配置和最大帧长的一致性。 |
| A-03 | 提供中控板规范，覆盖其主机与从机角色、命令范围、故障位、物理层和符合性。 |
| A-04 | 提供回路阻抗采集与端口控制板规范，含命令范围、故障位、物理层和符合性。 |
| A-05 | 提供储能控制板规范，含命令范围、故障位、物理层和符合性。 |
| A-06 | 提供贴靠阻抗采集板规范，含命令范围、故障位、物理层和符合性。 |
| A-07 | 核对各节点的通用命令支持级别和不支持清单，供主机固化命令表。 |
| A-08 | 确认是否有产品级事件或周期上报，并提供命令码分配。 |
| A-09 | 提供板卡上电顺序、中控板开始握手的时刻和整机就绪或自检窗口。 |
| A-10 | 确认产品级应答时限覆盖项及取值。 |
| A-11 | 确认会话、心跳、掉线和上传模式是否有整机级覆盖。 |
| A-12 | 补充 PSG 之外的各从机期望软件完整版本与协议版本映射。 |
| A-15 | 确认整机的失败处置和异常上报策略。 |
| A-16 | 核对主机是否按节点查对应板卡的故障字位定义及处置规则。 |

## 已裁决条款定位

以下条目依据 2026-11-18 的文档批注、后续确认，以及 2026-11-19 的跨文档一致性审查与执行计划记录（.scratch/grill/findings-consistency.md、exec-plan.md）。

| ID | 裁决 | 正式条款 |
| --- | --- | --- |
| PSG-04 | 两种启动源均检查两路电压、两路电流阈值和序列参数。R波触发已使能时，还须有成功写入的 0x36 参数。 | [SRS F-004](srs/pulse-gen-firmware.md#21-启停与触发)、[协议状态矩阵](spec/board-pulse-gen.md#命令状态矩阵) |
| PSG-05 | 掉线由主机判定，PSG 通过重新握手执行板内恢复。 | [板卡协议 §2](spec/board-pulse-gen.md#重复握手与恢复)、[ECSP §5.3](spec/ecsp-general.md#53-异常场景处理) |
| PSG-06 | PRM-604 作废，编号保留。 | [PRM 登记表](params.md#6xx-系统非功能)、[SRS F-014、F-015](srs/pulse-gen-firmware.md#21-启停与触发) |
| PSG-07 | PRM-203 同时用于本版软件发布版本和未写入的硬件版本。空白板 0x04 读回 `0.0.0.0`。上电校验版本记录，缺失或校验失败时将对应默认值连同校验码写回 Flash。 | [PRM 登记表](params.md#2xx-标识与版本)、[SRS I-024](srs/pulse-gen-firmware.md#32-命令与载荷)、[E-009](srs/pulse-gen-firmware.md#5-异常与安全需求e-) |
| PSG-08 | 中途停止或串级超时后，按本轮已取得的有效样本计算 0x31 电流。 | [SRS F-043、F-046](srs/pulse-gen-firmware.md#24-电流采样齐样与结算)、[采样与判定](design/current-settlement.md#采样与判定) |
| PSG-09 | 0x3D 成功清除电流异常后，0x31 双通道恢复已停止状态和普通电流读数，保留已完成触发次数。 | [SRS I-021](srs/pulse-gen-firmware.md#32-命令与载荷)、[E-004](srs/pulse-gen-firmware.md#5-异常与安全需求e-) |
| PSG-10 | PSG 的 0x03、0x05 可在已握手的正常业务会话中写入。 | [板卡符合性矩阵](spec/board-pulse-gen.md#6-符合性矩阵)、[ECSP §8](spec/ecsp-general.md#8-安全要求) |
| PSG-11 | 0x34 按整数 ns 配置，0x35 原样读回。标定驱动时长及各脉冲间隔按实际 HRTIM 周期取最近整数计数，半计数向上取整。外推点采用 T-001 和 T-006 中较严的容差。 | [SRS F-024、F-025](srs/pulse-gen-firmware.md#22-出波)、[T-001、T-006、T-009](srs/pulse-gen-firmware.md#4-时序需求t-) |
| PSG-13 | Q16 确认以循环 DMA 的每个半区对应一串，前台收到串结束和齐样事件后优先结算。 | [采样与判定](design/current-settlement.md#采样与判定)、[SRS F-041](srs/pulse-gen-firmware.md#24-电流采样齐样与结算) |
| PSG-14 | Q17 确认分别验收 ECSP 半包判定规则和实板接收计时精度。 | [UART 半包计时与验收](design/uart-reception.md#半包计时与验收)、[SRS T-011](srs/pulse-gen-firmware.md#4-时序需求t-) |
| ECSP-01 | 0x03 适用于动作类。0x11 表示当前请求与在途操作互斥，PSG 普通运行忙通过 0x31 确认。 | [ECSP §5.1](spec/ecsp-general.md#51-应答码定义)、[PSG 状态矩阵](spec/board-pulse-gen.md#命令状态矩阵) |
| ECSP-02 | 命令卡片定义载荷和特有条件。应答码、公共检查次序和状态拒绝优先级统一按 §5。 | [ECSP §5.1、§5.2](spec/ecsp-general.md#51-应答码定义) |
| ECSP-03 | 空闲心跳的发送完成间隔固定为 1000 ms。主机从该节点最近一帧有效应答或上报收齐时计 3000 ms，恰在截止时刻收齐的有效帧优先刷新计时。 | [ECSP §6.2](spec/ecsp-general.md#62-心跳监控)、[PRM-111 至 PRM-113](params.md#1xx-时序与会话) |
| ECSP-04 | 上电只主动探测一轮，失败后在 PRM-108 剩余时间内等握手上报。超时进入掉线，后续由有效握手上报触发接纳。 | [ECSP §4.2.1](spec/ecsp-general.md#421-主机上电操作) |
| ECSP-05 | 半包从候选帧头收齐起计 50 ms，后续字节不续期。候选失效后从其帧头下一字节重扫，沿用原接收时刻。 | [ECSP 半包计时](spec/ecsp-general.md#半包计时与重新同步)、[SRS T-011](srs/pulse-gen-firmware.md#4-时序需求t-) |
| ECSP-06 | 板卡声明物理层能力，产品发布每条链路的生效配置并核对两端一致。 | [ECSP §2.1](spec/ecsp-general.md#21-物理层参数)、[PSG 链路配置](spec/product-cardiac-pfa.md#3-psg-链路配置) |
| ECSP-07 | 超过 PRM-109 的长耗时操作禁止受理。 | [ECSP §4.4](spec/ecsp-general.md#44-长耗时命令处理机制) |
| HW-02（通道绑定） | FLT4 接 COMP1（负通道），FLT5 接 COMP3（正通道）。 | [MCU 故障与保护](hw/mcu-pinmap.md#243-故障与保护) |
| PSG-15 | Q18（2026-11-19）确认五层架构：按职责从上到下分为应用、运行控制、基础服务、板级执行和 MCU 驱动五层，分层、依赖方向、源文件目录及外设接口见架构文档。 | [固件架构](design/firmware-architecture.md#psg-v3-固件架构) |
| PSG-16 | PRM-605 作废，编号保留。 | [PRM 登记表](params.md#6xx-系统非功能)、[SRS N-003](srs/pulse-gen-firmware.md#6-非功能需求n-) |
| PSG-17 | 上传模式上电默认关闭，0x0F 开启；重复握手（I-012）恢复上电默认（关闭）；已建会话内的故障上报不受上传模式关闭影响（会话例外优先）。 | [SRS F-047](srs/pulse-gen-firmware.md#24-电流采样齐样与结算)、[SRS I-012](srs/pulse-gen-firmware.md#32-命令与载荷)、[ECSP 0x0F](spec/ecsp-general.md#0x0f-控制上传模式设置类) |
| PSG-18 | 上电自检、0x30 受理时出波前自检、触发间歇内自检与 0x3D 执行锁存清除后等待 5 ms（PRM-512）再重读锁存，仍有效则置故障字 bit0，清除成功则清除 bit0。 | [SRS E-003](srs/pulse-gen-firmware.md#5-异常与安全需求e-)、[SRS I-021](srs/pulse-gen-firmware.md#32-命令与载荷)、[MCU PG9](hw/mcu-pinmap.md#1-引脚分配) |
| PSG-19 | 掉线后 R波触发运行可长时间无主机控制持续出波，裁定记录为安全缺陷。板卡侧独立安全兜底：无主机控制下连续出波轮数/时长上限，超限自动停波并进入安全态，上限待安全分析。 | [GitHub Issue #1](https://github.com/zhengjy926/pfa-psg/issues/1)、[SRS F-011、F-012](srs/pulse-gen-firmware.md#21-启停与触发)、[PRM-313、PRM-314](params.md#3xx-序列与触发) |

| 主题 | 正式条款 |
| --- | --- |
| 启停与杆控制 | [SRS 功能需求](srs/pulse-gen-firmware.md#2-功能需求f-) 中 F-012、F-028 至 F-034、F-048 至 F-050；[协议状态矩阵](spec/board-pulse-gen.md#命令状态矩阵) |
| 欠流、过流、串级截止与诊断 | [SRS F-035 至 F-047](srs/pulse-gen-firmware.md#24-电流采样齐样与结算)、[E-001 至 E-005](srs/pulse-gen-firmware.md#5-异常与安全需求e-)；[0x31、0x3C 及故障字](spec/board-pulse-gen.md#31-命令总表) |
| 0x3D 清除与重复握手 | [SRS I-012、I-021](srs/pulse-gen-firmware.md#32-命令与载荷)；[板卡重置入口](spec/board-pulse-gen.md#重复握手与恢复) |
| 命令区、应答、长耗时控制、故障位归属与帧长 | [ECSP 命令与机制](spec/ecsp-general.md#3-命令集合)；[PSG 帧长](spec/board-pulse-gen.md#帧长和载荷上限) 与 [PSG 故障字](spec/board-pulse-gen.md#4-故障字位分配) |
| 输出电平、PB4 优先级和工具链 | [MCU HRTIM](hw/mcu-pinmap.md#24-hrtim1)、[NVIC](hw/mcu-pinmap.md#28-nvic)；[SRS N-001](srs/pulse-gen-firmware.md#6-非功能需求n-) |
| A-01 显控板地址、A-13 版本不符处置 | [产品通信配置 §2](spec/product-cardiac-pfa.md#2-地址版本与板卡规范)、[§4](spec/product-cardiac-pfa.md#4-版本不符处置) |
| A-14 扩展应答码归属 | [ECSP §3](spec/ecsp-general.md#3-命令集合)、[ECSP §5.1](spec/ecsp-general.md#51-应答码定义) |
