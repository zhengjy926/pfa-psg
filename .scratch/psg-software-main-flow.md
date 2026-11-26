# PSG 软件主逻辑流程图

按固件目标设计概括主流程。FreeRTOS 并行调度协议、脉冲引擎、电流结算等任务，图中的箭头表示业务推进顺序。

```mermaid
flowchart TD
    Boot([上电或复位]) --> Init["初始化板级资源和运行配置，保持双路无脉冲"]
    Init --> Idle["空闲：<br/>处理配置、查询和启动请求"]
    Idle --> Admit{"当前启停源的启动请求<br/>通过准入和自检？"}
    Admit -->|未通过| StayIdle([保持空闲])
    Admit -->|通过| Prepare["装载出波计划，完成杆选通准备"]
    Prepare --> Mode{"触发方式"}
    Mode -->|直接触发| Pulse["按计划出波和采样<br/>串级结算并决定后续串是否放行"]
    Mode -->|R 波触发| Wait["等待有效 R 波<br/>按配置延迟启动"]
    Wait --> Pulse
    Pulse -->|本轮正常完成| Result["更新完成计数和结果<br/>按上传模式上报"]
    Result --> More{"还需下一轮 R 波触发？"}
    More -->|是| Interval["触发间歇，执行出波前自检"]
    Interval -->|通过| WaitAgain([返回等待有效 R 波])
    Interval -->|失败，终止后续轮| BackToIdle([返回空闲])
    More -->|否| BackToIdle
```

- **中途停止或串级失约**。封锁后续输出，结束运行并回到空闲。
- **欠流或过流**。停波并进入电流异常态，`0x3D` 清除成功后回到空闲。有效端口停止和过流关断由硬件直接执行，软件随后处理状态和诊断。

[固件需求](../docs/srs/pulse-gen-firmware.md) · [任务与调度](../docs/design/firmware-architecture.md) · [脉冲引擎](../docs/design/pulse-engine.md) · [实现缺口](../docs/implementation-gaps.md)
