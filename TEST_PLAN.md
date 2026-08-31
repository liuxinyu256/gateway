# CH579 实机测试计划

基线: master @ dc23251 (2026-08-27) · 平台 CH579M0 + FreeRTOS · 工具链 Keil(主)/CMake(参考)
前置: PC 模拟器冒烟已通过 (`[OK] closed-loop smoke test passed`)
图例: P0 必测不过不往下走 / P1 核心功能 / P2 健壮性 / P3 稳定性

---

## T0 编译烧录与静态核对

| # | 项目 | 步骤 | 通过标准 |
|---|------|------|----------|
| 0.1 | [P0] Keil 全量编译 | Rebuild gateway.uvprojx | 0 Error; warning 逐条过目, 与 git 记录一致 |
| 0.2 | [P0] **定时器中断优先级补丁** | 见附A-R1。`timer_ch579.c` 只 Enable 没 SetPriority, 复位默认级 0 违反 `configMAX_SYSCALL`(=1)。先改码再上板 | TMR0-3 均 `NVIC_SetPriority(x, 1)` 后再 Enable |
| 0.3 | [P0] 引脚核对 | 对照原理图: UART0 RX=PB4(TTL上拉入), TX=PB7, DE=PA1; UART1 debug PA8(RX)/PA9(TX); 485 收发器 A/B、偏置电阻 | 实物接线 = hvac_init.c/debug.c 配置 |
| 0.4 | [P1] 485 收发器 /RE 接法 | 查图纸: /RE 若恒接地(接收常开)则自发自收, 见附A-R2 | DE 与 /RE 同源控制; 否则记录为已知问题 |

## T1 上电 Bring-up

| # | 项目 | 步骤 | 通过标准 |
|---|------|------|----------|
| 1.1 | [P0] 系统启动 | 烧录, UART1@115200 接终端 | 看到 `[GW] boot`, 调度器起后不再回车刷屏(不死循环/不 hardfault) |
| 1.2 | [P0] 调度存活 | 静置 1min, 观察任务运行 | rx/tx/gwstate/idle/tmrsvc 均被调度, 无任务饿死表现 |
| 1.3 | [P0] 单字节收 | USB-485 工具发 1 字节, UART1 加临时日志或示波器看 PB4 | 字节进入 ring, `bus_mark_rx_busy` 生效(bus.busy=1), 日志计数 +1 |
| 1.4 | [P0] 单帧发 | 临时注入一帧测试查询(6B), 示波器抓 TX+PA1 | 帧完整发出; DE 发送前拉高、`STA_TXALL_EMP` 后拉低 |

## T2 FreeRTOS ISR 路径 (模拟器未覆盖分支)

| # | 项目 | 步骤 | 通过标准 |
|---|------|------|----------|
| 2.1 | [P0] 帧完成→任务唤醒 | 连发多帧, UART1 打印 receive_queue 入队/出队计数 | 每次 5ms 封包后接收任务即时处理; `receive_queue_drop_cnt`=0 |
| 2.2 | [P0] tx_done→gap 定时器 | 发完一帧观察 gap_timer 启动(`xTimerStartFromISR`) → EVENT_BUS_IDLE → pump | 连续两帧间实测间隔 ≈ gap_ms; 无丢事件 |
| 2.3 | [P1] ISR 版 tick | 在 `bus_mark_idle`(bus.c:68 `FromISR` 路径) 打点 | gap_until 数值正确递增, 不出现回绕/负差 |
| 2.4 | [P1] 任务栈高水位 | `uxTaskGetStackHighWaterMark` 定期经 UART1 打印(rx/tx/gwstate 各 256 words) | 余量 > 20%; 注意 `module_handle_rx` 还有 128B 局部 buf |
| 2.5 | [P2] 中断负载 | 逻辑分析仪测 UART0/TMR0 ISR 单次时长 | 单次 < 100us 量级; 高流量下 SysTick 无明显抖动 |

## T3 RS485 半双工时序 (示波器双通道: TX 数据线 + PA1)

| # | 项目 | 步骤 | 通过标准 |
|---|------|------|----------|
| 3.1 | [P0] 尾字节完整 | 抓发出帧最后字节展开位级波形 | 最后字节 stop bit 完整无截断; DE 拉低发生在 stop bit 之后 |
| 3.2 | [P0] gap 实测 | 连续两次发送, 测静默间隔 | ≈3ms(35000/9600, bus.c:33), 误差 ≤ 1 tick(1ms); 对照空调协议要求可调 |
| 3.3 | [P0] 双机制不重复触发 | 快速连发 ≥10 帧, 加计数器打印 on_done 次数 | on_done 次数 == 发送帧数(见附A-R3 双检测并发隐患) |
| 3.4 | [P1] 占线不抢发 | 对端持续回数据期间下发控制命令 | `sender_pump` 因 `bus_is_idle()==false` 不出队; 对端帧结束后下一帧才启动 |
| 3.5 | [P1] 冲突恢复 | 制造对发碰撞(两只设备同发) | DE 释放正常, 下一次收发均能恢复, 不需复位 |

## T4 接收封包 (超时策略, timeout_ticks=5, 硬件 tick 1ms)

| # | 项目 | 步骤 | 通过标准 |
|---|------|------|----------|
| 4.1 | [P0] 正常成帧 | 发 6B 帧, 字节间隔 << 5ms | 恰好 1 帧, len==6, 内容无误 |
| 4.2 | [P0] 帧界阈值 | 分别用 2/4/6/10ms 字节间隙发送 | >5ms 断成两帧, ≤5ms 保持一帧(边界一致性); 与目标协议实际字节间隔比对留裕量 |
| 4.3 | [P1] ring 溢出 | 发 >128B 或噪声灌流 | 有溢出标志/丢弃行为且下帧恢复同步, 不越界写 |
| 4.4 | [P1] 半帧放弃 | 只发前 3 字节后停发 | 超时弃帧(to_reset 链路)总线回到空闲, 后续通信正常 |
| 4.5 | [P2] 上电毛刺 | 反复上电/插拔 485 总线 | 噪声封包被当垃圾丢弃; 不残留 bus.busy 卡死 |

## T5 总线状态机卡死恢复

| # | 项目 | 步骤 | 通过标准 |
|---|------|------|----------|
| 5.1 | [P1] 首字节挂起 | 注入 1 字节后停止(timer 应停), 观察多久释放 | 由 5ms 超时封包路径或 bus_on_rx_complete 释放; 发送恢复可用 |
| 5.2 | [P2] 极端卡死探测 | 临时屏蔽 TMR0 中断模拟丢失, 观察发送通道 | 出现"永久 busy 不再发"即证实附A-R4, 记录并评估加总线看门狗 |
| 5.3 | [P2] 反复中止 | 发送中途拔 A/B 线 20 次 | 每次重插后均自动恢复, 计数器可解释 |

## T6 队列深度与丢帧计数

背景负载事实: timeout_timer 每 50ms 无条件投 1 个 EVENT_TIMEOUT + poll 每 200ms 1 个 PERIODIC + BUS_IDLE 事件。

| # | 项目 | 步骤 | 通过标准 |
|---|------|------|----------|
| 6.1 | [P1] 基线零丢 | 空载跑 30s, UART1 周期打全部 drop 计数 | send/receive/state/frame_queue(cmd,norm) 计数全 0 |
| 6.2 | [P1] 命令风暴 | 以 100ms 间隔连发 gateway_send_cmd × 50, 再 20ms × 50 | cmd_q(深2)按设计溢出返回错误可感知; 计数增长可控可解释 |
| 6.3 | [P2] 全速饱和 | 收发同时满负荷 5min | 无 hardfault/重启; 队列水位与 drop 达到稳态而非线性恶化 |
| 6.4 | [P2] state_pending 合并 | 同模块高频 update_state | 同一模块 pending 不堆叠, 观察 gwstate 通知频率合理(state_event_drop_cnt=0) |

## T7 状态链路端到端

| # | 项目 | 步骤 | 通过标准 |
|---|------|------|----------|
| 7.1 | [P1] 真实 AC 回帧解析 | 接真空调, 让品牌层解析回帧 → ac_module_update_state | gateway_state_t 各字段正确(电源/模式/温度/风速) |
| 7.2 | [P1] 观察者通知 | 注册临时观察者经 UART1 打印 | 状态变化即通知, 未变不通知(memcmp 判等生效) |
| 7.3 | [P1] 下行闭环 | 观察→改温→观察 | 新 set_temp 真实写回空调并生效 |
| 7.4 | [P2] 空调离线 | 拔掉空调, 下发命令 | 协议层超时策略生效(EVENT_TIMEOUT 每 50ms 驱动), 错误上报合理 |

## T8 长跑稳定性

| # | 项目 | 步骤 | 通过标准 |
|---|------|------|----------|
| 8.1 | [P3] 24h 连跑 | 业务满节奏循环 + UART1 每 60s 输出健康快照(drop 计数/高水位/自由堆) | 无重启无 hardfault; 所有计数曲线收敛; 温漂引起的 baud 偏差不致误码率上升 |
| 8.2 | [P3] 计数器溢出审计 | uint16 计数器 65356 回绕 | 打印侧对回绕做差值处理, 曲线不跳变 |
| 8.3 | [P3] 断电记忆性 | 上电初始态核查 | 冷启动状态干净(gateway_init memset), 无上一轮残留状态误报 |

---

## 附A 评审发现的风险项 (上板前决策)

| ID | 风险 | 依据 | 建议 |
|----|------|------|------|
| R1 | **TMR0-3 中断优先级未设置**, 默认级 0 高于 `configMAX_SYSCALL_INTERRUPT_PRIORITY`(1<<6)。而 TMR 回调链(on_timeout→frame_done_cb)调用 xQueueSendFromISR/xTimerStartFromISR/xTaskGetTickCountFromISR, CM0 移植下属违规, 可能表现为随机-hardfault 或队列错乱 | timer_ch579.c:31-46 只有 EnableIRQ; lib/FreeRTOS/FreeRTOSConfig.h:41 | 合入一行 SetPriority(=1)。UART 已设 1(uart_ch579.c:49) 是对齐做法 |
| R2 | 485 收发器若 /RE 恒使能接收: 自发帧被回环喂进 receiver, to_on_byte 不区分方向, 会污染 RX 缓冲/制造假帧 | 原理图核对项 T0.4; receiver_timeout.c 无方向过滤 | 硬件同源控制; 否则固件发帧期间 discard 回环 |
| R3 | TX_COMPLETE 双检测并发(UART ISR 内联 + 1ms tx_poll 定时器任务上下文)对 wait_tx_complete 的 check-and-clear 非原子, 存在双重 on_tx_complete 理论窗口 | sender.c:115-153, module.c:201-210 | 用 T3.3 实测计数; 如需加固将清标志改为临界区 |
| R4 | bus.busy 一旦因异常(半帧+定时器停)悬挂, sender_pump 永久不出队, 无任何兜底释放 | bus.c/bus_mark_rx_busy 仅靠 rx 完成/超时回收 | 视 T5.2 结果决定是否加"距最近总线活动 N ms 强制 idle"看门狗 |
| R5 | sim_app.c 遗留旧签名调用 `timer_hw_create(&rx_timer,0)`(现行 API 为返回指针 `timer_t*`), 该分支当前编译不可达, 属死代码 | sim_app.c:184 vs timer.h:49 | 低优先级清理, 防止日后改动踩坑 |
| R6 | debug_printf 为阻塞式 UART 写(vsnprintf 128B 栈 buf + 逐字节轮询发送), 高频日志会拖慢事件实时性 | app/debug.c:42-52 | 测试期尽量低频使用; 量产精简或改 FIFO+ISR 发送 |

## 附B 执行记录

每完成一项在下方追加: `[日期] 编号 结果(通过/失败/阻塞) 数据摘要 (示波器截图序号/日志文件名)`。缺陷单开 issue, 引用本表编号。
