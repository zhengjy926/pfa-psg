# PSG 文档导航

首次接手，从固件需求确认行为与验收边界，再按任务查协议、参数或 MCU 配置。已有需求 ID、命令码、PRM 或信号名时，直接从下表进入对应正文。

| 要查什么 | 从哪里开始 | 定位方式 |
| --- | --- | --- |
| 实现范围、固件行为和验收 | [SRS](srs/pulse-gen-firmware.md#0-查阅约定) | F- 功能、I- 接口、T- 时序、E- 异常与安全、N- 非功能 |
| PSG 线上接口 | [命令索引](spec/board-pulse-gen.md#31-命令总表) · [状态矩阵](spec/board-pulse-gen.md#命令状态矩阵) · [故障字](spec/board-pulse-gen.md#4-故障字位分配) · [支持声明](spec/board-pulse-gen.md#6-符合性矩阵) | 按命令码查字段和报文，按业务状态查受理与应答 |
| ECSP 公共规则 | [帧与编码](spec/ecsp-general.md#2-帧与编码) · [通用命令](spec/ecsp-general.md#31-命令总表) · [会话和事务](spec/ecsp-general.md#4-通信机制) · [应答码](spec/ecsp-general.md#51-应答码定义) · [时限与覆盖](spec/ecsp-general.md#6-时序) | 先确认通用规则，再查板卡声明的差异 |
| 整机联调 | [产品通信配置](spec/product-cardiac-pfa.md) | 按链路确认主从角色，按节点查地址、期望版本和版本不符处置 |
| 参数值、单位、范围、默认值 | [PRM 登记表](params.md) | 按 ID 或号段查数值，再读引用条款确认适用条件 |
| 引脚、复用、外设、DMA 与中断 | [MCU 引脚表](hw/mcu-pinmap.md#1-引脚分配) · [外设配置](hw/mcu-pinmap.md#2-外设配置) | 按信号或引脚查绑定，沿表内需求 ID 和命令码反查 |
| 固件分层与架构决策 | [固件架构](design/firmware-architecture.md) · [FreeRTOS 任务化调度](adr/0001-bare-metal-cooperative-scheduling.md) · [双 Bank Flash 布局](adr/0002-dual-bank-flash-layout.md) | 查文件目录、接口依赖、状态归属、调度模型及持久化布局 |
| UART 设备接口与收发 | [UART 收发设计](design/uart-reception.md) | 查设备接口、IDLE 中断、DMA、半包与发送完成 |
| 启停裁定与运行状态归属 | [脉冲引擎设计](design/pulse-engine.md) | 查 ECSP 请求交接、端口停止和过流并发处理 |
| ADC 样本归属、串级结算和截止时间验证 | [电流采集与串级结算设计](design/current-settlement.md) | 按处理阶段核对实现，并回查 SRS 的截止条件 |
| 术语、冲突和待补输入 | [术语表](../GLOSSARY.md) · [疑点与待补项](ambiguities.md) | 按术语语境或受影响的条款定位 |
| 实现缺口、台架待办与复核处置 | [实现缺口登记](implementation-gaps.md#0-查阅约定) | 按 GAP 编号、ticket 或状态查 |

## 常用阅读路径

- **实现启停或杆控制**。先看 SRS F-001 至 F-016、F-027 至 F-034 和 F-048 至 F-050，再查 0x30、0x40、0x42 和状态矩阵，最后对照 PB4、PB6 与 PD8 至 PD13。
- **定位状态包或故障上报**。先看 0x31/0x3C 或 0x11/0x12 的字段，接着看 SRS 电流与异常条款，最后沿 ADC、COMP、HRTIM 及过流锁存信号查硬件。
- **联调接入中控板**。先核对产品拓扑、地址和版本，再查 ECSP 握手与应答规则，最后核对 PSG 帧长、命令支持与例外。

## 更新与核对

- 固件行为和验收写入 SRS，线上字段、应答与支持写入对应协议，地址分配与期望版本写入产品配置。更新量值时先改 PRM 登记，检查引用和示例。硬件绑定集中在 MCU 映射。
- 保留仍有效的需求 ID、命令码与 PRM 编号，修改标题或移动条款时同步核对相关链接。
- 冲突与缺口登记在[文档疑点](ambiguities.md)。裁决后回填正式条款并记录出处。

在仓库根目录执行。第二条验证已展开的 ECSP 示例帧，符号载荷的 CRC 用 `CRC` 表示。

```powershell
python -X utf8 tools/doc_check.py
python -X utf8 tools/frame_check.py --verify 'docs/spec/*.md'
```
