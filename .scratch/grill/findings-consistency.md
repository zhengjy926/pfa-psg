# 跨文档一致性审查发现（术语表 / 参数表 / 引脚表 / 疑点页 / 导航）

## 方法

1. 用 read 逐份通读 6 份必读文件：`GLOSSARY.md`、`docs/params.md`、`docs/ambiguities.md`、`docs/hw/mcu-pinmap.md`、`docs/README.md`、`docs/agents/domain.md`。
2. 通读 `docs` 全目录其余 12 份正文文档（srs/pulse-gen-firmware.md、spec/board-pulse-gen.md、spec/ecsp-general.md、spec/product-cardiac-pfa.md、design/firmware-architecture.md、design/pulse-engine.md、design/uart-reception.md、design/current-settlement.md、design/embedded-layering-research.md、adr/0001、adr/0002、agents/issue-tracker.md、agents/triage-labels.md）。
3. 用 grep 对全库做交叉检索：GLOSSARY 全部 _Avoid_ 词、全部 PRM-xxx 引用、全部引脚名（P[A-G]\d+）、TODO/TBD/FIXME/待定/暂定/未定/后续/占位 等占位词。
4. 运行仓库自带校验工具：`python -X utf8 tools/doc_check.py`（PRM 引用、断链、SRS 重复 ID、占位残留）与 `python -X utf8 tools/frame_check.py --verify 'docs/spec/*.md'`（57 帧示例帧 CRC/长度）。
5. 用 `.scratch/grill/_consistency_check.py` 统计：PRM 定义 vs 引用差集、PRM 编号缺口，并按 GitHub 锚点规则逐条校验 305 个链接锚点。
6. 对每处疑点回读正文档落段确认语义后再定级。行号以当前文件为准。

核对通过项（无问题）：

- 305 个文档内锚点全部有效；0 个断链文件；0 个未定义 PRM 被引用；SRS 需求 ID 无重复。
- 57 个示例帧长度与 CRC16-MODBUS 全部正确（含 board-pulse-gen、ecsp-general 全部数字示例）。
- 全库无 TODO / TBD / FIXME / XXX / 未定 / 暂定 / 占位（语义性）残留；唯一"待定"为 ambiguities PSG-03 已登记的 PB4 停波验收阈值。
- 参数数值链全部自洽：PRM-010(9)+PRM-013(441)=PRM-011(450)；PRM-013=1+20×22；请求最大帧 449=9+440；看门狗 0.941–1.085 s 与 LSI 29.5–34 kHz、/8、3999 换算一致；PLL 8/2×85=340 MHz、340/2=170、340/7=48.57 MHz 全对；PRM-714 65534@340 MHz≈192.7 µs、最小 PWM≈5.19 kHz 全对；T-011 半包验收输入（49/50/90/91 ms）与 PRM-115 自洽。
- 引脚表无一处双功能冲突；文档引用的引脚全部存在于 mcu-pinmap，且信号、AF、需求归属一致。

---

## 1 术语表（GLOSSARY）审查

### 1.1 定义与实际用法冲突（含 _Avoid_ 违规）

| # | 严重度 | 问题 | 位置 |
| --- | --- | --- | --- |
| T-01 | P2 | 使用被禁词「上升沿/下降沿」：board-pulse-gen 协议状态矩阵"忽略本次上升沿/等新的上升沿"（×2）；SRS F-012"PB4 下降沿"（×2）；ambiguities PSG-03"PB4 下降沿"。术语表要求用 正向过渡/负向过渡。 | docs/spec/board-pulse-gen.md:555；docs/srs/pulse-gen-firmware.md:40；docs/ambiguities.md:11（对比 GLOSSARY.md:25,37,41） |
| T-02 | P2 | 「脉宽/输出脉宽」高频使用但无定义，且与「脉冲持续时间」的 _Avoid_(脉宽) 摩擦：SRS F-024/F-025/T-001/T-006 与 design/current-settlement 全部用"输出脉宽/功率输出脉宽/驱动脉宽"指实测功率输出宽度——这是与配置值「脉冲持续时间」不同的概念，需要正式术语；GLOSSARY「标定」定义自己也用"目标输出脉宽"。 | docs/srs/pulse-gen-firmware.md:57-58,151,156；docs/design/current-settlement.md:16；GLOSSARY.md:147 |
| T-03 | P2 | 「事件帧」违反「主动上报帧」的 _Avoid_：ecsp-general §5.2"事件帧沿用 §2.5 的主动上报规则"；GLOSSARY「结果事件」定义自身也用"独立事件帧"。 | docs/spec/ecsp-general.md:1052；GLOSSARY.md:430 |
| T-04 | P2 | 「硬件过流」违反「过流锁存」的 _Avoid_：mcu-pinmap §2.8"硬件过流停波独立于 NVIC 调度"；firmware-architecture"硬件过流保护沿硬件通路执行"。 | docs/hw/mcu-pinmap.md:229；docs/design/firmware-architecture.md:45 |
| T-05 | P3 | 「帧格式」违反「帧类型」的 _Avoid_："会话以 0x01 建立或重建，帧格式见命令卡片"。 | docs/spec/ecsp-general.md:866 |
| T-06 | P3 | 「总线」违反「链路」的 _Avoid_："同一产品总线"；GLOSSARY「板卡地址」定义自身也用"产品总线"。 | docs/spec/ecsp-general.md:52；GLOSSARY.md:362 |
| T-07 | P3 | 「上位机」违反「主机」的 _Avoid_（SRS 查阅约定与交付范围两处）。 | docs/srs/pulse-gen-firmware.md:17,21 |
| T-08 | P3 | 示例帧占位符用被禁词「SYNC」表示事务序号。 | docs/spec/ecsp-general.md:426,467,621 |
| T-09 | P3 | 「去重缓存/去重 FIFO/事务去重」与规范术语「事务序号缓存」（_Avoid_ 应答缓存）并存漂移：SRS I-012、firmware-architecture、PRM-028 均用"去重"。 | docs/srs/pulse-gen-firmware.md:123；docs/design/firmware-architecture.md:10；docs/params.md:49 |
| T-10 | P3 | 「放电电流波形」违反「出波」的 _Avoid_(放电)。 | docs/ambiguities.md:12 |
| T-11 | P3 | 「事件驱动状态机」违反「脉冲引擎」的 _Avoid_(运行状态机)。 | docs/adr/0001-bare-metal-cooperative-scheduling.md:3 |
| T-12 | P3 | 裸用「串/群」（首串/末串/该串/每串/每群/群末）违反 _Avoid_(串/群)。 | docs/srs/pulse-gen-firmware.md:69-70,84,87,89；docs/design/current-settlement.md:8-9,18,20-21；docs/hw/mcu-pinmap.md:74 等多处 |
| T-13 | P3 | 「板卡协议」昵称漂移：全库用"板卡协议"指 board-pulse-gen.md，术语表只定义「板卡规范」（_Avoid_ 板级协议，但未收"板卡协议"）。 | docs/srs/pulse-gen-firmware.md:59,124,134；docs/hw/mcu-pinmap.md:31 等多处 |

### 1.2 术语表定义与规范条款不一致（实质性偏差）

| # | 严重度 | 问题 | 位置 |
| --- | --- | --- | --- |
| T-14 | P2 | 「脉冲引擎」运转子态写作「等待触发」，全库实际用词为「等待 R 波」（SRS F-006/F-030/F-048/F-050、F-045、板卡 0x32）。 | GLOSSARY.md:194 vs docs/srs/pulse-gen-firmware.md:34,68,73,75,96 |
| T-15 | P2 | 「会话状态」枚举缺「等待连接」：ecsp-general 从机侧 4 处使用「等待连接」（§4.2.2、应答码 0x10、§5.2 第 3 步），而表 4-1 与 GLOSSARY 只有「未连接」。同一状态两个名字未说明主/从侧关系。 | docs/spec/ecsp-general.md:917,922,930,1028,1056 vs GLOSSARY.md:398、ecsp-general.md:874 |
| T-16 | P2 | 「受理启动」定义漏一个拒绝条件：GLOSSARY 只列"故障字 bit1 置位，或任一通道过流检出成立"，E-002 还有第三条"硬件过流锁存仍有效"。 | GLOSSARY.md:198 vs docs/srs/pulse-gen-firmware.md:180 |
| T-17 | P2 | 「结果事件」定义断言"再次开启上传模式，或握手把模式设回开启，都不补发"——ECSP 规范正文（C-0x0F-01、§4.6）没有"不补发"条款，术语表自行添加了规范外行为。 | GLOSSARY.md:430 vs docs/spec/ecsp-general.md:727,1004 |
| T-18 | P3 | 「会话」定义自身使用「断链」（链路故障的 _Avoid_ 词）。 | GLOSSARY.md:394 |
| T-19 | P3 | 「标定」定义用被禁的"脉宽"（见 T-02）。 | GLOSSARY.md:147 |

### 1.3 高频术语缺失定义（应补）

| # | 严重度 | 缺失术语 | 使用频率/位置 |
| --- | --- | --- | --- |
| T-20 | P2 | 串级结算 / 串级超时（串级结算超时）——SRS F-038/F-041/F-043、板卡 C-0x3C-05、两个设计文档高频使用；而 GLOSSARY 已定义的「结算超时」全库零使用，形同死词。 | docs/srs/pulse-gen-firmware.md:84,87,89；docs/spec/board-pulse-gen.md:386；docs/design/current-settlement.md:1,11,22 |
| T-21 | P2 | 群级结算 / 群级结算等待（F-040、PRM-117/118/119 的依据）。 | docs/srs/pulse-gen-firmware.md:86；docs/params.md:75-77 |
| T-22 | P2 | 诊断电流、普通电流读数（F-046、C-0x31-05、E-004 的结算口径）。 | docs/srs/pulse-gen-firmware.md:97,123,182；docs/spec/board-pulse-gen.md:145 |
| T-23 | P3 | 齐样等待（E-001）、过流快照（PRM-410/F-043）、输出脉宽（T-02）、驱动时长（F-024/T-009）、IGBT（current-settlement 16,19；术语表只有 MOSFET 延迟术语）、半包/重同步（T-011/§5.3 高频）、上传门控（C-0x11-06）、控制码（0x30/0x08/0x0C/0x0F 字段）、握手上报、标定表。 | 多处（见上列） |

### 1.4 死术语（定义存在但全库无使用）

| # | 问题 | 位置 |
| --- | --- | --- |
| T-24 | 状态出现、瞬态、过渡、50% 参考电平、过渡出现时刻——仅存在于 GLOSSARY 内部定义链，SRS 未使用这套波形分析词汇。 | GLOSSARY.md:14-33 |
| T-25 | 异常串——定义并指向 SRS §2.4，但 SRS 通篇不用该词。 | GLOSSARY.md:305 |
| T-26 | MOSFET关断延迟时间——F-031 只引用导通延迟（PRM-508），关断延迟无任何引用。 | GLOSSARY.md:183 |

---

## 2 参数表（params.md）审查

### 2.1 数值/单位/默认值冲突

未发现硬性数值冲突（全部换算链核对通过，见"方法"节）。发现 2 处语义/表述问题：

| # | 严重度 | 问题 | 位置 |
| --- | --- | --- | --- |
| P-01 | P2 | 0x30 停止与启动在「空闲 + 启停输入源为端口」下的受理不对称且无说明：启动回 0x12（模式不允许），停止却"受理，保持空闲"（无操作）。F-002 只授权选定源"开始或中途停止运行"，空闲时的无操作停止为何豁免未解释。 | docs/spec/board-pulse-gen.md:547-549 |
| P-02 | P3 | 故障字 bit1 清除条件写"复位命令，或清除电流异常（0x3D）"——"复位命令"未限定（0x07 还是过流锁存复位？），与 E-003/I-021（只写 0x3D 做过流锁存复位并清位）口径不一致，且恰好撞上 GLOSSARY「复位」同形异义提醒。 | docs/spec/board-pulse-gen.md:568 vs docs/srs/pulse-gen-firmware.md:132,181 |

### 2.2 无人引用的参数（orphan）

| # | 问题 | 位置 |
| --- | --- | --- |
| P-03 | PRM-030（应答码/结果码/控制码/模式字段宽度）除登记表外无人引用（ecsp-general §5.1 用 PRM-026，命令卡片各自写"1 字节"）。 | docs/params.md:51 |
| P-04 | PRM-711（ADC 循环 DMA 通道）无人引用（pinmap §2.7 直接引 PRM-738）。 | docs/params.md:185 |

（PRM-606 作废编号保留，无人引用属预期，不算问题。）

### 2.3 编号缺口未标注

| # | 问题 | 位置 |
| --- | --- | --- |
| P-05 | PRM-605 缺失且无"作废/保留"说明（PRM-604、PRM-606 均有作废标注，605 被跳过）。 | docs/params.md（6xx 段） |
| P-06 | PRM-316～319 缺失无说明（3xx 从 315 直接跳 320）。 | docs/params.md（3xx 段） |
| P-07 | SRS 需求编号缺口同样未标注：I-022、I-026、E-010、E-011、N-005 缺失。 | docs/srs/pulse-gen-firmware.md |

### 2.4 登记表自身格式问题

| # | 问题 | 位置 |
| --- | --- | --- |
| P-08 | 「性质」列出现表头未定义的新取值：作废、示例、硬件特性、发布值/默认（表头只声明固定/上电默认/可配/派生四种）。 | docs/params.md:4 vs 53,85,167,195,203 |
| P-09 | PRM-114 名称"握手重发频率"与实际用途（从机周期发送握手上报的频率，非重发）不符。 | docs/params.md:72 vs docs/spec/ecsp-general.md:917 |
| P-10 | PRM-503 值"±(1% + 10 µs)"含单位但单位列为空。 | docs/params.md:150 |
| P-11 | PRM-203 一值双义（软件发布版本 + 硬件版本默认值同为 0.0.0.0），产品配置中控板期望的软件版本也是 0.0.0.0——空白板 0x04 读回值与真实写入值无法区分，建议版本值避开默认值。 | docs/params.md:85；docs/spec/product-cardiac-pfa.md:25 |
| P-12 | GLOSSARY「回路阻抗采集与端口控制板」称"整机中地址为 0x03"，未指明是板卡地址（产品地址恰也是 0x03，易混）。 | GLOSSARY.md:287（PRM-205=0x03、PRM-212=0x03） |

---

## 3 引脚表（mcu-pinmap.md）审查

| # | 严重度 | 问题 | 位置 |
| --- | --- | --- | --- |
| PIN-01 | — | 核对结论：无引脚双功能冲突；全库引用的引脚（PA0/1/2/3/8/9、PA13/14、PB4/5/6/9、PC4/5、PD8–13、PF0/1、PG7/8/9）与 pinmap §1 一致；COMP/DAC/HRTIM/EXTI/NVIC 归属均一致。 | — |
| PIN-02 | P3 | E-006"六杆依赖外部下拉保持关断"（明确下拉）与 pinmap"杆驱动 PD8 至 PD13 与锁存复位 PG9 在此期间由板级上下拉约束"（笼统上下拉）表述不一致。 | docs/srs/pulse-gen-firmware.md:184 vs docs/hw/mcu-pinmap.md:58 |
| PIN-03 | P3 | R 波输入 PB6 的需求列写"F-006、F-009、F-031"，F-009（触发间歇位置）与 R 波无直接关系，应为 F-010（触发间歇期间的 R 波忽略）。 | docs/hw/mcu-pinmap.md:42 |

---

## 4 ambiguities.md 时效性审查

逐条回查结论：

- **全部 24 条（PSG-01/02/03/12 待验收 + HW-01~05 + A-02~A-12/A-15/A-16 + 已裁决 21 条）引用的文件与锚点均有效**（305 锚点全通过，无失效行号/章节）。
- **待补条目仍全部成立**：PSG-01/02/03/12 的待补数据（采样误差、群级预算实测、PB4 停波延迟、功率关断路径）在 SRS/设计文档中仍标"待台架验证"；HW-01（DAC 默认/出厂值）、HW-02（TB 寄存器、FLT4P/5P 极性）、HW-03（ORE/DMA 错误恢复）、HW-04（双 Bank 实测）、HW-05（半包计时精度）在 pinmap/uart-reception 中仍为"待核实/待验证"，无一条可删除。
- **PSG-03 的"PB4 停波验收阈值待定"仍成立**（T-005 只定义了过流两条停波时限，未给 PB4 停止时限）——保留正确。

结构性问题：

| # | 严重度 | 问题 | 位置 |
| --- | --- | --- | --- |
| A-01 | P3 | ID 复用：PSG-01/02/03/12 同时出现在「PSG 行为与验收」和「已裁决条款定位」两张表，同一 ID 承载两种内容（验收项 vs 裁决结论），检索易混。 | docs/ambiguities.md:9-12 vs 50-52 |
| A-02 | P3 | A-14 完全无记录（A-01、A-13 在"主题"表有裁决出处，A-14 连编号去向都无交代）。 | docs/ambiguities.md:28-42 |
| A-03 | P3 | 2026-11-19 的 Q18 裁决（五层架构）未登记进「已裁决」表——本页声明"裁决后回填正式条款并记录出处"，Q15/Q16/Q17 都登记了，Q18 漏登。 | docs/design/firmware-architecture.md:3 |
| A-04 | P3 | PRM-606 作废只有 params.md 自注，无裁决记录（PSG-06 只裁决了 PRM-604）。 | docs/ambiguities.md:55；docs/params.md:168 |
| A-05 | P3 | 遗漏的明显疑点未登记：①0x30 停止在空闲+端口源下的受理不对称（见 P-01）；②故障字 bit1 清除条件"复位命令"歧义（见 P-02）；③「等待连接」与「未连接」状态命名分裂（见 T-15）；④IGBT vs MOSFET 器件术语（见 T-23）。 | — |

---

## 5 全库占位词检索

| 词 | 结果 |
| --- | --- |
| TODO / TBD / FIXME / XXX | 0 处 |
| 待定 | 1 处（ambiguities PSG-03"PB4 停波验收阈值待定"，已登记） |
| 暂定 / 未定 | 0 处 |
| 待补 / 待验证 / 待核实 / 待确认 / 待测量 | 均与 ambiguities HW-01~05 及 pinmap §0.1/§2.3/§2.4.2/§2.4.3/§2.6.2、uart-reception 的"见待补项"互指，闭环一致 |
| 占位 | 1 处正常语义（PRM-715"上电占位"） |
| 后续 | 全部为正常语义（"后续扩展/后续字节/后续上报"），无未决占位 |

结论：占位管理干净，唯一未定项已登记在案。

---

## 6 docs/README.md 导航准确性

| # | 问题 | 位置 |
| --- | --- | --- |
| R-01 | 通过：导航表列出的 12 个入口文件全部存在，描述与实际内容相符（SRS/协议/PRM/引脚/架构/设计文档的行文主题均对应）；两个工具命令可执行且当前全绿（doc_check OK；frame_check 57 帧 0 错）。 | docs/README.md:5-17,33-35 |
| R-02 | P3 | "符号载荷的 CRC 用 CRC 表示"与工具实际机制（{{CRC}} 占位符，verify 模式校验已展开的数字帧）表述不一致；且"在仓库根目录执行。"一句悬空无主语。 | docs/README.md:31 |

---

## 7 问题总数统计

| 类别 | P2 | P3 | 合计 |
| --- | --- | --- | --- |
| 术语表 | 11 | 15 | 26 |
| 参数表 | 1 | 11 | 12 |
| 引脚表 | 0 | 2 | 2 |
| ambiguities | 0 | 5 | 5 |
| README | 0 | 1 | 1 |
| **总计** | **12** | **34** | **46** |

（另有核对通过项 5 类，见"方法"节；P0/P1 级 0 项——无会导致实现分叉的硬冲突。）

## 8 Top 10 最重要问题（按影响排序）

1. **T-15 会话状态命名分裂**：「等待连接」（从机侧）未进 GLOSSARY「会话状态」枚举，与「未连接」并存——影响 ECSP 实现状态机与文档互指（ecsp-general.md:917/922/930/1028/1056 vs GLOSSARY.md:398）。
2. **T-02「脉宽」概念缺失且违反 _Avoid_**：SRS 四个验收条款（F-024/F-025/T-001/T-006）全部用"输出脉宽"作验收口径，术语表却禁"脉宽"且未定义该概念（srs:57-58,151,156）。
3. **T-16「受理启动」定义漏条件**：术语表两条件 vs E-002 三条件（缺"硬件过流锁存仍有效"），照术语表实现会漏拒绝分支（GLOSSARY.md:198 vs srs:180）。
4. **T-17「结果事件」术语表私加规范外条款**："上传模式重开不补发"在 ECSP 正文无出处，可能误导从机实现（GLOSSARY.md:430）。
5. **T-14 运行子态名分裂**：「等待触发」（术语表）vs「等待 R 波」（SRS/板卡全库），影响状态机文档一致（GLOSSARY.md:194）。
6. **P-01 0x30 停止/启动受理不对称**：空闲+端口源下启动回 0x12 而停止受理，无条款解释，联调时主机行为难定（board-pulse-gen.md:547-549）。
7. **T-20 结算超时死词 + 串级超时无定义**：SRS/板卡/设计文档核心判定词"串级超时/串级结算"全无术语定义，术语表定义的"结算超时"零使用（srs:87,89；board:386）。
8. **P-02 故障字 bit1 清除条件歧义**："复位命令"未限定（0x07 还是锁存复位），与 E-003/I-021 只写 0x3D 的口径不一致（board:568）。
9. **T-04 硬件过流违规用词**：mcu-pinmap 安全章节与架构文档用被禁词"硬件过流"指过流锁存，安全语义最需精确（pinmap:229；firmware-architecture:45）。
10. **T-01 上升沿/下降沿违规**：协议状态矩阵与 SRS F-012（核心验收条款）用被禁词描述 PB4 启停与触发语义（board:555；srs:40）。

## 9 数值/引脚/术语冲突清单摘要

- **数值**：0 个硬冲突；57 帧示例帧 CRC/长度全对；PLL/看门狗/帧长/采样时长换算链全对。2 个语义问题（P-01 受理不对称、P-02 复位命令歧义）+ 2 个 orphan PRM（PRM-030、PRM-711）+ 编号缺口（PRM-605、PRM-316~319、I-022/026、E-010/011、N-005）。
- **引脚**：0 个冲突；2 个表述问题（PIN-02 下拉 vs 上下拉、PIN-03 需求列 F-009 应为 F-010）。
- **术语**：_Avoid_ 违规 12 类（上升沿/下降沿、脉宽、事件帧、硬件过流、帧格式、总线、上位机、SYNC、去重缓存、放电、状态机、裸"串/群"）；定义与条款不一致 4 处（等待触发、等待连接、受理启动、结果事件不补发）；缺定义 14+ 高频术语；死术语 8 个。
