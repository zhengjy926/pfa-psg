# 架构设计优化报告（第二阶段 / grill-with-docs）

> 日期：2026-11-19 · 承接第一轮一致性审查（commit 6ab9a46）。本阶段目标：提出更好的架构设计并落地，不是纠错。

## 1. 机会地图

`arch-opt-map.md`：基于已提交版文档产出 **20 条优化机会**（明确收益 12 / 需权衡 8；实时与安全时序 8 + 资源 6 + 可维护性 6），并对第一轮"优化 7 条"逐条复审（3 条已消解、4 条并入）。

## 2. 裁决结果（R2 20 条 + R3 数值 8 条，全部定案）

- **实时**：停波单所有者矩阵 + 故障复位锁存化；ADC 三级回退阶梯（L3 经核查不适用）；串级结算方案 A（事件驱动 + 任务通知）；configMAX_SYSCALL=4 + ISR WCET<2µs + 停波延迟公式；自检快速路径 + 挂起式等待；监视器 100ms/2 次/零共享；REP=N-1 硬件定界（含预取写法）；DMA 固定缓冲 + NDTR 锚定。
- **资源**：RAM 预算表 ≤32KB + 栈金丝雀；双槽快照环形覆盖；去重条目压缩 ≤128B；UART 环形 ≥1KiB + 应答优先级；Bank2 布局显式化 + 裁定不磨损均衡；标定表 ≤256B 修订路径 + 0x34 WCET 登记。
- **可维护**：六任务（调度+ECSP 合并为协议任务）+ 职责矩阵；事件结构体化；双时基 + 注入 + 替身 + 失约钩子；裁定链单一所有者；app_config.h 集中登记；drv/ 九驱动 config/data/ops 全量抽象（参考 Zephyr/TinyUSB/grblHAL）。

## 3. RM0440 硬件事实核查（rm0440-facts.md）

1. REPR 运行中可写，更新事件时转移、RPTR 每 REP 事件按有效值重载 → **提前一串预取写入**；
2. G474 突发控制器不写 CMP/PER/REP → 群边界改软件预装载或 burst DMA；TAREP 在 REP=0 时每周期触发；N×T_PWM 小于突发周期须 TABM=1；
3. PB4 可复用 HRTIM1_EEV7（AF13）硬件停波 typ56/max66 ns，与 EXTI4 互斥 → 记录为可选硬件联动（默认保持 ISR 方案）；
4. 双 ADC 同时采样上限 **52 MHz**（DS12288 Table 66）→ L3（56.67MHz）不适用，维持 48.57MHz。

## 4. 执行与验证

- 四个编辑子代理并行改写：firmware-architecture.md（163→208 行重构）、current-settlement.md、uart-reception.md、pulse-engine.md、ADR 0001/0002、embedded-layering-research.md、SRS、params（+PRM-607/608）、pinmap、GLOSSARY（+3 术语）。
- 集成修正 12 处：RM0440 门控项落笔、DMA 归属矛盾（pinmap:74/PSG-13）、L3 不适用、PRM-715 派生、PRM-708 依据等。
- **验证**：doc_check OK（161 PRM / 126 术语 / 105 需求行 / 0 问题）；frame_check 57 帧 0 错误。

## 5. 台架行动项（已随文档登记）

- T-005 锁存化重验；T-007 L2 全范围复测；L1 相位锚定验证；NDTR 快照与最后转换完成同拍竞态；REPR 转移/重载同拍次序（最坏再滞后一串）。
- 已采纳升级：PB4→HRTIM1_EEV7 硬件端口停波（PRM-513，typ56/max66ns）；L3 仅单 ADC 场景备选。

## 6. EEV7 硬件端口停波升级（2026-11-19 追加裁决）

- PB4 复用 AF13=HRTIM1_EEV7（引脚源、双边沿、滤波 0000）；EEV7→EXTEVNT7 复位双路输出，停波延迟 typ 56/max 66 ns（DS12288 Table 62），不依赖中断。
- 端口事件（启动正向/停止负向）由 EEV7→TIMA CPT1 捕获经 HRTIM1_TIMA_IRQn 入队，ISR 按 PB4 电平判向；事件捕捉延迟不影响停波。
- 协议源（0x3E）时屏蔽 EEV7 复位与捕获；与过流故障通路（FLT4/5←COMP1/3）正交。
- 落点：pinmap §1/§2.4.6/§2.8、params PRM-513/730、SRS F-003/F-012/T-005、firmware-architecture §1/§5/场景3、pulse-engine。
