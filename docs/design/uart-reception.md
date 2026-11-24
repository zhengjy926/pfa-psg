# UART 设备抽象与收发设计

## 设备接口

UART 采用 Zephyr 设备模型的静态子集。设备对象包含三个部分。 [Zephyr 设备模型](https://docs.zephyrproject.org/latest/kernel/drivers/index.html)

- `config` 保存实例的只读硬件配置，含 USART 实例及 DMA 和中断绑定。
- `data` 保存实例私有的运行状态，含接收进度、错误状态和静态缓冲区。
- `ops` 是只读的类型化 UART 操作表，实现相同的实例共享一份操作表。

板级启动代码按编译期 `CONFIG_PSG_PROTO_UART` 显式装配和初始化 UART 设备，并将类型化句柄交给 ECSP。发布版默认绑定 USART2，单板调试版绑定 USART1，共用同一份 STM32 驱动实现。移植时由目标平台实现同一 UART 设备接口。 [SRS I-025、N-001](../srs/pulse-gen-firmware.md)

ECSP 通过设备句柄批量读取接收字节。每次读取由调用方给定容量，设备立即返回实际读取量。用于 ECSP 的设备适配层还提供与字节位置对应的接收时序信息，接收时刻随字节携带。设备应报告接收溢出或故障造成的数据流断点，由 ECSP 重新同步。 [SRS N-003](../srs/pulse-gen-firmware.md#6-非功能需求n-)

## STM32 接收

STM32 适配层启用 USART 空闲（IDLE）中断，优先验证循环 DMA 接收，落地为零拷贝环形缓冲：每路 ≥1 KiB（2 × 450 B 最大帧长 + 重同步余量），容量按下限推导式「11.5 KB/s 到达率 × 最坏服务间隔」与 T-011 最坏负载联立核定（115200 bps ÷ 10 位/字节 ≈ 11.5 KB/s）。IDLE 中断记录接收进度和线路空闲观察，连续输入期间也要更新接收进度并检测缓冲区覆盖。IDLE 与 DMA 接收进度经队列通知 UART/ECSP 任务；溢出行为写死为丢最旧字节 → 计接收溢出 → 触发重新同步（HW-03），字节接收时刻随字节携带、不受溢出影响。UART/ECSP 任务按最长服务间隔和单次处理预算读取数据，再执行解帧与命令分派。 [STM32G4 UART 扩展驱动](https://github.com/STMicroelectronics/stm32g4xx-hal-driver/blob/master/Src/stm32g4xx_hal_uart_ex.c)

IDLE 和 DMA 半传输、完成事件用于更新接收进度。到达时序优先验证用单调计时器采样 DMA 写入进度的方案。实板将驱动记录的接收时刻与线路上字节收齐时刻对比，覆盖最坏负载、连续输入、短帧和 DMA 回绕。计时误差带待测量后确认。溢出和接收错误后的恢复按 HW-03 验证。 [硬件待补项 HW-03、HW-05](../ambiguities.md#硬件输入)

## 半包计时与验收

ECSP 以候选帧头字节的接收时刻起算 50 ms 半包时限。接收时刻随字节携带（环形缓冲逐字节记录），溢出丢弃最旧字节不改变存留字节的接收时刻。候选失效后，从其帧头的下一字节重扫缓冲区。若在缓冲区发现后一个候选帧头，以该字节原来的接收时刻计算它自己的截止时刻。前台读取或重扫时刻不参与计算。 [ECSP 半包计时](../spec/ecsp-general.md#半包计时与重新同步)

协议测试按时间戳注入模型验证半包计时：业务层统一使用 timebase 单调时基（定宽计数、回绕安全比较），FreeRTOS tick 仅用于任务延时；主机测试经 timebase 的 set_time/advance 注入接口以脚本时钟推进，注入可控的字节接收时间戳，验证截止前、恰在截止时刻和截止后的收齐结果，以及重扫后计算出的截止时刻。实板按 T-011 单独核对计时精度，误差带用于判读台架结果，协议计算仍使用 50 ms 半包时限。 [SRS T-011](../srs/pulse-gen-firmware.md#4-时序需求t-)

## 发送

ECSP 任务将应答和主动上报放入同一发送队列，发送顺序按优先级裁定：应答头插（或分双队列、发送泵按应答优先出队），主动上报仅在应答预算有余时放行。心跳（空闲链路每 1000 ms 完成一次发送）与 0x3C、0x11 上报计入应答预算核算。应答预算计入接收服务等待、请求处理、在途帧的剩余发送时间和排在应答前面的帧的发送时间。按最坏负载核算队列容量，按 T-011 逐命令验收。 [SRS T-011](../srs/pulse-gen-firmware.md#4-时序需求t-) · [ECSP 应答时限](../spec/ecsp-general.md#61-命令应答时限)

UART 设备非阻塞地受理发送，同一设备同时处理一笔在途发送。STM32 使用普通 DMA 搬运发送数据，以 USART 发送完成标志 `TC` 确认并记录帧在线路上的发送完成时刻。ECSP 使用该时刻更新心跳基准，`0x07` 应答发送完成后才执行复位。 [MCU DMA 配置](../hw/mcu-pinmap.md#27-dma) · [ECSP 从机心跳](../spec/ecsp-general.md#622-从机) · [ECSP 系统复位](../spec/ecsp-general.md#0x07-系统复位动作类)

## 参考实现

- 设备实例借鉴 Zephyr 的只读配置、私有运行状态和按设备类型提供操作接口的组织方式。 [Zephyr 设备模型](https://docs.zephyrproject.org/latest/kernel/drivers/index.html)
- 上层读取借鉴 RT-Thread 串口 V2 的非阻塞批量读取。 [RT-Thread 串口 V2](https://github.com/RT-Thread/rt-thread/blob/master/components/drivers/serial/dev_serial_v2.c#L568-L655)
- STM32 适配层优先复用 STM32CubeG4 提供的 USART、DMA 和 IDLE 实现，按 PSG 的连续接收、半包时序、错误恢复和静态资源要求验证。 [STM32CubeG4](https://github.com/STMicroelectronics/STM32CubeG4) · [STM32G4 UART 扩展驱动](https://github.com/STMicroelectronics/stm32g4xx-hal-driver/blob/master/Src/stm32g4xx_hal_uart_ex.c)
