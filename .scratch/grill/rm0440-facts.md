# RM0440 / DS12288 硬件事实核查（供架构优化裁决）

> 核查日期：2026-11-19。取证方法：从 ST 官网下载官方 PDF 原文（本地留存于 `.scratch/grill/stpdf/`：
> `rm0440.pdf`=RM0440 Rev 9、`ds12288-stm32g474qe.pdf`=DS12288 Rev 6、`an4539.pdf`=AN4539 Rev 5），
> 用 pypdf 抽取文本层后逐条核对，条款号与页码均为 Rev 9/Rev 6 原书编号。
> 注意：RM0440 旧版（Rev≤6）中 HRTIM 是第 26 章，Rev 9 是第 28 章；Rev 9 把 REPR 改名为
> **HRTIM_REPxR（字段 REPx[7:0]）**，与项目文档里的 `REPR`/`TIMxREPR` 是同一寄存器。

---

## 1. REPR 更新：运行期间可写；预装载生效时机 = 更新事件（重复事件）

**结论（确定）**：REPR（Rev 9 名 HRTIM_REPxR）运行期间**随时可写**（普通 RW 字段，无写保护/锁定位）。
写入何时生效由 PREEN（HRTIM_TIMxCR bit 27）决定：
- **PREEN=0（"更新禁用"，当前配置）**：写直达有效寄存器，立即生效；运行中把 PREEN 清零会立刻把预装载内容灌入有效寄存器。
- **PREEN=1（"更新使能"）**：写入进预装载寄存器，**仅在更新事件时**转移。定时器更新事件可选
  `TxREPU=1`（HRTIM_TIMxCR bit 17）配置为"计数器 roll-over 且重复计数器=0"——**即重复事件本身**。

但关键限制在**重复计数器（RPTR）的重载时机**：RPTR 不可读，在 TxCEN 使能时用 REPxR 初始化，此后每次计数器清零递减，到 0 产生 REP 中断/DMA 请求并**自动从当前有效 REPxR 重载**（auto-reload）。
因此**串间窗口（REP 事件之后）写 REPR，无论 PREEN=0 还是 =1，都只影响"下下串"**：
第 k 串结束（E_k）时 RPTR 已按旧值重载完毕；窗口内写入的新值最早在 E_{k+1}（下一次更新/重复事件）生效 → 作用于第 k+2 串。
要让第 k+1 串恰好 N_{k+1} 个脉冲，必须**提前一串**写（在 E_k 之前、即第 k 串脉冲期间写入）。
RM0440 未写明 E 事件瞬间"预装载转移"与"RPTR 重载"的内部次序（TIM1/TIM15 章有明确"先转移后重载"表述，HRTIM 章没有），最坏情形再多滞后一串——**此点无法从手册确认，需实测**。

**证据**（RM0440 Rev 9）：
- §28.5.18（p.1019）HRTIM_REPxR："This register holds the repetition period value. This register holds either the content of the preload register or the content of the active register if preload is disabled."；字段 REPx[7:0] rw，地址偏移 Block A: 0x098。
- §28.3.11（p.942）："By default, PREEN bits … are reset and the registers are not preloaded: any write directly updates the active registers. If PREEN bit is reset while the timer is running and preload was enabled, the content of the preload registers is directly transferred into the active registers."
- §28.5.12（p.1006-1007）HRTIM_TIMxCR，PREEN（bit 27）："0: Preload disabled: the write access is directly done into the active register / 1: Preload enabled: the write access is done into the preload register"。
- §28.5.12（p.1008）TxREPU（bit 17）："Register update is triggered when the counter rolls over and HRTIM_REPx = 0"；UPDGAT（bits 31:28，p.1006）：0010 = "The update occurs on the update event following the DMA burst transfer completion"。
- §28.3.4（p.896）Repetition counter："The repetition counter … cannot be read, but solely programmed with an auto-reload value in the HRTIM_REPxR register … initialized with the content of the HRTIM_REPxR register when the timer is enabled … any time the counter is cleared … the repetition counter is decreased. When it reaches zero, a REP interrupt or a DMA request is issued … If the HRTIM_REPxR register is set to 0, an interrupt is generated for each and every period. For any value above 0, a REP interrupt is generated after (HRTIM_REPxR + 1) periods."
- Table 244（p.943）：Timer x 预装载寄存器列表含 HRTIM_TIMxREP（=REPxR），更新源含 Software / TIMx repetition event / TIMx reset event / Burst DMA event 等。
- §28.3.11（p.944）官方同构示例（"Second converter"）：TAREPU=1 → "the transfer from preload to active registers is done as soon as the timer A repetition event occurs"，并示范用 TAUDIS/TDUDIS/TEUDIS 挂起转移、多寄存器一致性写入。

**对架构决策的直接影响**：
- 串间 ≥1ms 窗口（PRM-305）内写 REPR **可行**，但必须按"提前一串"的流水线编排（波形表本来就预知，第 k 串内写第 k+1 串的 N−1 即可），不能指望窗口内写入作用于紧随其后的一串。
- "更新禁用→更新使能"的改动 = `TACR.PREEN=1` + `TACR.TxREPU=1`（PRM-715 相关行）。若同时改 CMP1/2/3 参数，PREEN=1 会一并把它们变成"重复事件时生效"，时序统一，正符合串级更新需求；多寄存器写入期间可用 TAUDIS 挂起转移保证一致性。
- 建议在评审会列出实测项：E 事件瞬间 REPxR 转移与 RPTR 重载次序（决定是 +1 串还是 +2 串滞后）。

---

## 2. 突发模式：G474 的突发控制器不写参数寄存器；"Timer A 重复"触发语义

**结论（确定）**：
- 触发源"Timer A 重复" = HRTIM_BMTRGR bit 8 **TAREP**。REP≠0 时重复事件每 **REPxR+1** 个周期产生一次（§28.3.4）；**REPxR=0 时每周期产生一次**——即当前配置（REP=0）下突发是**每个周期**被触发一次，不是每群一次。
- **G474 的突发模式控制器只控制输出空闲/运行**（自己的计数器 + BMCMPR/BMPER），**不会写任何定时单元的 CMP/PER/REP 寄存器**。BMPREN（BMCR bit 10）只作用于 BMCMPR/BMPER 自身的预装载。"群边界把下一群 CMP 参数突发重载"必须另走：①软件写预装载（§28.3.11，配合 TxREPU=1 在重复事件转移）；②**burst DMA**（§28.3.23：BDMUPR/BDTxUPR/BDMADR，BRSTDMA+UPDGAT 三种用例，其中 BRSTDMA=10+UPDGAT=0010 = "update on the update event following the DMA burst transfer completion" 保证整组一致更新）；③ RM0440 官方"突发模式仿真"（§28.3.15 末尾，CMP1CxR 复合寄存器 {REP;CMP1}，重复事件触发 DMA 连写两字）。
- **约束条款（必须满足）**：§28.3.15（p.956）："Burst mode is not re-triggerable … in single-shot mode, the triggers are ignored until the current burst completion including run periods (HRTIM_BMPER+1 cycles)." → 下一串的 REP 触发到达时若上一次单次突发还没结束（含运行期），触发被丢弃。当 N×T_PWM < 突发周期时必然丢触发（N=1 与空闲≥1ms 组合就属此类）。
  - 化解：设 **TABM=1**（突发空闲期停止并把定时器计数器保持复位，§28.3.15，p.955），空闲期内无 roll-over → 无 REP 事件 → 空闲结束后重新计满 N 个周期才产生下一次触发，触发不再被丢弃；空闲时长（≥1ms，PRM-305）由 BMCMPR+1 个突发时钟（fHRTIM÷512≈332kHz）实现。
  - **无法确认点**：TABM=1 时"计数器保持复位"是否算一次 counter clear 而额外递减重复计数器（§28.3.4 的递减条件涵盖 reset event，但未涵盖突发空闲保持复位），RM0440 未明确，需实测。
- **突发模式下写 CMP/REP/PER 预装载寄存器可靠**：RM0440 无任何"REPR 更新与突发模式冲突"条款；唯一"burst mode 激活期间不得修改"的是输出管理位（IDLEMx/IDLESx 等，§28.3.15 Note p.955、§28.5.x Notes），与参数寄存器无关。与 Q1 相同的"提前一串"时序适用：空闲窗口内写入的 CMP 在 E_{k+1} 转移、第 k+2 串生效；要第 k+1 串生效需在 E_k 前写入（如用 E_{k-1} 的重复事件 REPDE 触发 burst DMA，串内传输、E_k 转移——即 ST 社区官方回复（Pierre_Paris）描述的 UPDGAT 用法）。

**证据**（RM0440 Rev 9）：
- §28.5.65（p.1100-1101）BMTRGR：Bit 8 TAREP："The Timer A repetition event is starting the burst mode operation."（bit 29/27 还有 EEV7/TAEEV7 触发源）。
- §28.3.15（p.954-962）：Burst mode controller 全节（BMCMPR/BMPER、BMCLK、SINGLESHOT 语义、not re-triggerable、TxBM 计数器保持复位、BMPREN、"Burst mode emulation using a compound register"）。
- §28.3.23（p.985-986）："Burst DMA transfers"、"Several options are available once the DMA burst is completed … When the preload is enabled (PREEN bit set), there are 3 use cases:"（1. 独立于 DMA 的更新；2. BRSTDMA=01 突发完成即更新；3. BRSTDMA=10 突发完成后跟随的更新事件更新）。
- §28.3.11（p.943-944）：master update 选项 3/4（BRSTDMA=01/10）；Table 244 更新源含 Burst DMA event。
- §28.4 / §28.3.4（p.895-896）：roll-over 事件用途含 "To serve as a burst mode clock source or a burst start trigger"、"To decrement the repetition counter"。

**对架构决策的直接影响**：
- 现行配置（REP=0、触发源=Timer A 重复、SINGLESHOT）在硬件语义上=突发**每周期**触发一次，PRM-715 必须改为 REP=N−1（N≥1）才实现"每群一次"；同时 REP 中断（TIMA REP）随之恰好每群一次，符合设计意图。
- "下一群 CMP 参数由突发重载"不成立——必须改为 burst DMA（推荐 BRSTDMA=10+UPDGAT=0010，硬件化、整组一致）或软件预装载写入；突发控制器保留其真正的职责：群间空闲（≥1ms 停波窗口，TABM=1 配置）。
- 必须加设计约束：突发周期（BMPER+1 个突发时钟）< N×T_PWM，否则丢触发；建议 TABM=1 使 REP 计数与空闲互斥，并实测"空闲复位是否递减 RPTR"。

---

## 3. EXTI→HRTIM：EXTI 线不能进 EEV，但 PB4 引脚本身就是 HRTIM_EEV7 输入（AF13）

**结论（确定）**：
- HRTIM 外部事件（EEV）源是每通道一个 4:1 复用器（EExSRC[1:0]）：**Src1(00)=外部引脚 hrtim_eevx、Src2(01)=片内比较器 COMPx、Src3(10)=通用定时器 TRGO、Src4(11)=ADC 模拟看门狗**。源列表里**没有 EXTI 线**——EXTI4 作为中断线不能直接作 EEV 源，也没有任何 EXTI→HRTIM 直连通路。
- **但 PB4 引脚本身是 hrtim_eev7 的外部输入**：DS12288 Rev 6 Table 12（p.70）PB4 行 AF13 = **HRTIM1_EEV7**。把 PB4 由 GPIO 输入改为 AF13 后，端口停波信号直接进 HRTIM，可用作：
  - 输出置位/复位交叉开关源 **EXTEVNT7**（HRTIM_SETx1R/HRTIM_RSTx1R 等，§28.3.4 p.898：set/reset 源含 "The external events EXTEVNT1..10 (10 events)"）；
  - 计数器复位源（HRTIM_RSTxR，p.895 同列表）；
  - 突发触发源（BMTRGR bit 29 EEV7 / bit 27 TAEEV7）。
  - 硬件停波延迟（同步模式，滤波关）：DS12288 Rev 6 **Table 62**：EEV 数字输入→输出引脚 typ 56 ns / max 66 ns @ fHRTIM=170 MHz，抖动 ≤1 tHRTIM，最小脉冲宽度 7 ns。对比 Table 60 故障路径（typ 16/max 31 ns）慢一些，但远超 EXTI 中断软件路径（µs 级）。
- EEV7 **不能**作故障通道源：Table 228（p.888）G474 的 FLT1..6 片内 EEV 选项只有 EEV1..EEV6 muxout（FLT4←EEV4_muxout、FLT5←EEV5_muxout），EEV7 不在列；且 FLT4/FLT5 已被 COMP1/COMP3 占用（FLTxSRC=01，同一时刻只能选一个源）。端口停波的硬件联动应走"EEV7→输出/计数器复位"，而不是故障通道。
- 附带事实：EEV7 属 EEV6..10 组，无快速模式（EExFAST 仅 EEV1..5），但有数字滤波（HRTIM_EECR3 EExF，停波用 0000=不过滤）；源选择 EE7SRC=00、灵敏度 EE7SNS=下降沿、EE7POL=0。
- EXTI4 与 AF13 的取舍：RM0440 §9.3.8（p.351）："To use external interrupt lines, the port must be configured in input mode."——严格按手册，PB4 配成 AF13 后不再满足 EXTI 的 input-mode 要求（实践中 STM32 的 EXTI 线在 AF 模式下仍跟随引脚输入，但手册规范性表述如此）。若硬件联动落地，EXTI4 软件路径只能降级或放弃；若必须保留 EXTI4，则 PB4 保持输入模式、硬件联动不可用。

**证据**（RM0440 Rev 9 + DS12288 Rev 6）：
- §28.3.8（p.925-926）："These external events come from multiple sources … built-in comparators, digital input pins …, on-chip events for other peripheral (ADC's analog watchdogs and general purpose timer trigger outputs)"、"select up to 4 sources with the EExSRC[1:0] bits"。
- Table 223（p.887-888）：EEV1..10 各通道 Src1..4 明细（EEV1←PC12，EEV2←PC11，EEV3←PB7，EEV4←PB6，EEV5←PB9，EEV6←PB5，**EEV7←PB4**，EEV8←PB8，EEV9←PB3，EEV10←PC5/PC6；Src2 依次为 COMP2/4/6/1/3/2/4/6/5/7）。
- Table 228（p.888）：Fault inputs（FLT1..6 = 外部引脚 / COMP2/4/6/1/3/5 / EEV1..6_muxout）。
- §28.3.17（p.963-964）："FLTxSRC[1:0] bit selects the source of the fault signal, that can be either a digital input or an internal event (built-in comparator output)"。
- DS12288 Table 62（p.136）：HRTIM output response to external events 1-10 (Synchronous mode)：tLAT(DEEV) typ 56 / max 66 ns（fHRTIM=170 MHz），TJIT(EEV) ≤1 tHRTIM，tW(EEV) 7 ns。
- DS12288 Table 12（p.70）：PB4 行 AF 列表含 HRTIM1_EEV7（AF13），附加功能 UCPD1_CC2。

**对架构决策的直接影响**：
- PB4（EXTI4）负向过渡停波存在**真硬件联动路径**：PB4→AF13(EEV7)→EXTEVNT7→HRTIM_SETA1R/RSTA1R 与 HRTIM_SETA2R/RSTA2R 复位双路输出，端到端 ≈56-66 ns（不依赖中断/NVIC），满足"最快停波"。
- 代价：与 EXTI4 软件路径互斥（RM0440 §9.3.8 措辞），需裁决保留哪条；EEV7 与过流故障（FLT4/5←COMP1/3）正交、互不占用，可同时生效。

---

## 4. ADC 时钟上限（ADC1+ADC2 同时采样）：52 MHz（不是 60 MHz）

**结论（确定）**：DS12288 Rev 6（stm32g474qe.pdf）**Table 66 "ADC characteristics"（p.138-139）**，fADC 行：
- "Range 1, single ADC operation"：0.14–**60 MHz**；
- "**Range 1, all ADCs operation, single ended mode, VDDA ≥ 2.7 V**"：0.14–**52 MHz**；
- "Range 1, all ADCs operation, single ended mode, VDDA ≥ 1.62 V"：0.14–42 MHz；
- "Range 1, all ADCs operation, differential mode, VDDA ≥ 1.62 V"：0.14–56 MHz。

ADC1+ADC2 同时采样（本项目双通道同时采样，正/负通道分别接 ADC1/ADC2）属 "all ADCs operation, single ended mode" → **上限 52 MHz（VDDA≥2.7 V 前提）**；若 VDDA 在 1.62–2.7 V 之间则只有 42 MHz。

**对架构决策的直接影响**：PRM-708（ADC1/ADC2 时钟）必须 ≤52 MHz，不能沿用单 ADC 的 60 MHz 配置；PLL P（PRM-705）支路需按 52 MHz 重新核算，或保持现有频率并核实 ≤52 MHz。采样阶段时长 = PRM-710 ÷ PRM-708 随之变化，连带检查 [采样验证](../design/current-settlement.md) 的时序结论。

---

## 来源 URL

1. RM0440 Rev 9（ST 官方 PDF，HRTIM=第 28 章）：<https://www.st.com/resource/en/reference_manual/rm0440-stm32g4-series-advanced-armbased-32-bit-mcus-stmicroelectronics.pdf>
2. STM32G474 数据手册 DS12288 Rev 6：<https://www.st.com/resource/en/datasheet/stm32g474qe.pdf>
3. AN4539 HRTIM cookbook Rev 5（突发模式官方例程）：<https://www.st.com/resource/en/application_note/an4539-hrtim-cookbook-stmicroelectronics.pdf>
4. ST 社区官方回复（ST 员工 Pierre_Paris，burst DMA + UPDGAT 三种用例）：<https://community.st.com/stm32-mcus-products-25/hrtim-stm32h743-for-3-phase-control-shadowing-register-126602>
5. ST 社区：HRTIM 突发模式与重复计数器初始化问题（配置后需生成一次更新事件使 RPTR 装载）：<https://community.st.com/stm32-mcus-embedded-software-32/help-with-burst-mode-hrtim-160088>

## 无法确认项（供后续实测/咨询 ST）

1. 重复事件瞬间"预装载转移→有效 REPxR"与"RPTR 重载"的先后次序（决定 REPR 写入是 +1 串还是 +2 串滞后）。
2. TABM=1 突发空闲期"计数器保持复位"是否额外递减重复计数器。
3. PB4 配置为 AF13（HRTIM_EEV7）后 EXTI4 是否仍能产生中断（RM0440 §9.3.8 要求 input mode；实践中通常仍触发，但手册未承诺）。

本地取证副本：`.scratch/grill/stpdf/`（官方 PDF + 抽取文本 `rm0440.txt` / `ds12288.txt` / `an4539.txt`）。
