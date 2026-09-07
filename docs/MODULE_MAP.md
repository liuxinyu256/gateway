# MODULE_MAP.md — 模块职责 / 接口 / 事件 / 依赖

> 这是“模块地图”，不是代码全文。只记录每个模块：
> - 负责什么
> - 关键接口
> - 消费/产生什么事件
> - 依赖谁
> - 修改入口

---

## 1. 模块总览

| 模块 | 文件 | 职责 | 状态 |
|------|------|------|------|
| 模块框架 | `src/core/module.c/.h` | 双任务、事件队列、定时器、发送/接收公共流程 | 已实现 |
| 网关状态中心 | `src/core/gateway_device.c/.h` | 统一状态、状态事件、观察者 | 已实现 |
| AC 模块 | `src/modules/ac_module.c/.h` | 扫描/锁定/轮询/品牌状态机调度 | 已实现 |
| Debug 模块 | `src/modules/debug_module.c/.h` | 日志/命令/心跳/运行统计 | 已实现 |
| BLE 模块 | `src/modules/ble_module.c/.h` | BLE 状态镜像/上报骨架 | 骨架 |
| WiFi/无线/第三方 | `src/modules/*` | 预留 | TODO |
| AC 品牌协议 | `src/brands/*` | AC 协议解析/组帧 | ac_test 已实现 |
| AC 物理层 | `src/phy/ac_phy*.c` | UART/RS485 装配 | 已实现 |
| Debug 物理层 | `src/phy/debug_phy.c` | UART1 装配 | 已实现 |
| BLE 物理层 | `src/phy/ble_phy.c` | BLE 协议栈任务 | 已实现/可选 |
| 板级 | `src/bsp/*` | GPIO/通路/引脚配置 | A07S 已实现 |

---

## 2. 核心框架 module

### 2.1 职责

- 每个模块一条独立总线通道
- 创建 `send_task` / `receive_task`
- 创建 `send_queue` / `receive_queue`
- 创建 `poll_timer` / `tick_timer` / `gap_timer`
- 统一处理 RX 帧、周期事件、网关命令、总线空闲
- 调用子模块的 `module_ops_t` / `event_handler_t`

### 2.2 关键接口

| 接口 | 作用 |
|------|------|
| `module_init(m, cfg)` | 调用模块自己的 `ops->init` |
| `module_base_init(m, baud)` | 初始化 bus/队列并注册 module_id |
| `module_start(m)` | 创建任务、定时器、注册 IO 回调 |
| `module_set_handler(m, evt, ctx)` | 绑定事件表 |
| `module_rx_frame_done(m, len)` | 接收完成，投 EVENT_RX_FRAME |
| `module_tx_done_from_isr(m)` | 发送完成，启动 gap（ISR 版） |
| `module_send_frame(m, data, len, prio)` | 模块级投帧 |
| `module_send_gateway_cmd(m, cmd, val)` | 投网关命令 |
| `module_send_state_sync(m, state)` | 投完整状态同步 |
| `module_set_poll_period(m, ms)` | 改轮询周期 |
| `module_update_state(m, state)` | 更新状态并上报 |

### 2.3 事件

| 事件 | 去向 |
|------|------|
| `EVENT_PERIODIC_SEND` | send_task，调 `on_periodic_send` |
| `EVENT_RX_FRAME` | receive_task，读帧并调 `on_rx_frame` |
| `EVENT_GATEWAY_CMD` | send_task，调 `on_gateway_cmd` |
| `EVENT_SCAN_AC` | send_task，调 `on_scan` |
| `EVENT_TICK` | send_task，调 `on_tick` |
| `EVENT_BUS_IDLE` | send_task，调 `sender_pump` |
| `EVENT_AC_RX` | send_task，AC 内部使用 |
| `EVENT_SEND_FRAME` | send_task，通用投帧 |
| `EVENT_DEBUG_TX` | Debug 模块内部 |

---

## 3. 网关状态中心 gateway_device

### 3.1 职责

- 保存每个 module 的完整 `gateway_state_t`
- 状态变化事件队列，去重 pending
- 状态变化后：
  1. 通知观察者（Debug/BLE）
  2. 同步完整状态给其他模块

### 3.2 关键接口

| 接口 | 作用 |
|------|------|
| `gateway_init()` | 创建状态队列/gwstate 任务 |
| `gateway_module_state_update(id, s)` | 模块上报状态 |
| `gateway_module_state_get(id, out)` | 读取模块状态 |
| `gateway_send_cmd(id, cmd, val)` | 向下发命令 |
| `gateway_send_event(id, type)` | 向模块投事件 |
| `gateway_on_state_change(cb, ctx)` | 注册状态观察者 |
| `gateway_set_module(id, m)` | 注册模块实例 |
| `gateway_module(id)` | 获取模块实例 |

### 3.3 状态字段

```c
power, mode, set_temp, room_temp, fan, swing, error_code,
mode_caps, fan_caps, swing_caps,
temp_min, temp_max, temp_step, features
```

---

## 4. AC 模块

### 4.1 数据结构

```c
ac_module_t {
    module_t base;              // 继承模块框架
    brand_table;                // 品牌注册表
    brand_count;
    current;                    // 当前品牌
    scan_index;
    locked;
    rx_buf[128];
}
```

### 4.2 职责

- 管理 AC 品牌扫描 / 锁定
- 调用品牌 `protocol_ops` 组帧、解析
- 统一负责发送和定时
- 状态合并/上报
- 诊断日志

### 4.3 关键接口

| 接口 | 作用 |
|------|------|
| `ac_module_register(self, cfg)` | 绑定品牌并设置 handler |
| `ac_module_start_scan(self)` | 启动扫描 |
| `ac_module_lock(self)` | 手动锁定 |
| `ac_module_locked(self)` | 查询锁定 |
| `ac_module_current(self)` | 当前品牌 |
| `ac_module_send_frame(frame, len)` | 模块级发送 |
| `ac_module_set_poll_period(self, ms)` | 改轮询周期 |
| `ac_module_update_state(self, state)` | 更新完整状态 |
| `ac_module_publish_state(self)` | 上报状态 |

### 4.4 内部事件处理

| 事件 | 处理函数 | 说明 |
|------|----------|------|
| `EVENT_PERIODIC_SEND` | `ac_module_on_periodic_send` | 锁定后轮询；未锁定扫描 |
| `EVENT_SCAN_AC` | `ac_module_on_scan` | 启动/推进扫描 |
| `EVENT_RX_FRAME` | `ac_module_on_rx_frame` | 调品牌 rx_parse，锁定 |
| `EVENT_GATEWAY_CMD` | `ac_module_on_gateway_cmd` | 状态同步/控制 |
| `EVENT_AC_RX` | `ac_module_run_state_machine` | 调品牌 state_machine |
| `EVENT_SEND_FRAME` | `ac_ops_on_event` | 调 sender_send |

### 4.5 诊断日志

```text
[ac evt] rx: ...
[ac] brand locked id=1
[gw] module=0 power=...
[diag] up=... s rs485 dir tx=... rx=...
```

---

## 5. AC 品牌协议层

### 5.1 数据结构

```c
ac_brand_config_t {
    brand_id;
    phy_cfg;         // 物理层参数（波特率、超时等）
    protocol_ops;    // 协议操作
    ability;         // 能力
}
```

### 5.2 protocol_ops

| 接口 | 作用 |
|------|------|
| `on_scan(buf, max)` | 生成扫描帧 |
| `state_machine(event, state, tx, max, next_period_ms)` | 根据事件生成帧/决定下次周期 |
| `rx_parse(data, len, out)` | 解析应答帧，返回品牌事件 |
| `poll_period_ms` | 默认轮询周期 |

### 5.3 现有品牌

| 品牌 | 文件 | 状态 |
|------|------|------|
| `ac_test` | `src/brands/ac_test.c` | 已实现 Modbus RTU 测试协议 |
| `ac_gree` | `src/brands/ac_gree.c` | 空文件，未注册 |

### 5.4 新增 AC 品牌步骤

```text
1. 新建 src/brands/ac_xxx.c/.h
2. 在 ac_brand.h 的 AC_BRAND_LIST 加一行
   BRAND(ac_xxx, ac_xxx_cfg)
3. 实现：
   - rx_parse()
   - state_machine()
   - on_scan()
   - 配置 poll_period_ms
4. 定义 ac_xxx_cfg / phy_cfg
5. 编译烧录，实机验证扫描锁定
```

---

## 6. Debug 模块

### 6.1 职责

- UART1 115200 日志输出
- 命令输入
- 心跳 `alive`
- 状态观察者打印
- 运行时长/FreeRTOS task 统计

### 6.2 关键接口

| 接口 | 作用 |
|------|------|
| `debug_module_start()` | 启动 Debug 模块 |
| `log_printf(fmt, ...)` | 通用日志 |
| `log_hex_dump(tag, data, len)` | HEX 日志 |
| `log_event_enabled()` | 事件日志开关 |
| `log_rx_enabled()` | RX 日志开关 |
| `debug_uptime_s()` | 系统运行秒数 |

### 6.3 调试命令

| 命令 | 作用 |
|------|------|
| `alive` 开关由 `hb` 控制 | 心跳 |
| `rxlog` | 开关 AC RX 日志 |
| `evt` | 开关事件日志 |
| `perf` | CPU/RAM 统计 |
| `stat` | 队列丢弃统计 |
| `up` / `uptime` | 系统运行时长 |
| `task` | FreeRTOS 每个任务运行统计 |
| `ctrl` | 控制命令 |
| `state` | 状态 |
| `tx` | 测试发送 |

---

## 7. BLE 模块

| 项目 | 内容 |
|------|------|
| 文件 | `src/modules/ble_module.c`、`src/phy/ble_phy.c` |
| 模块 ID | 2 |
| 职责 | 镜像网关状态、状态变化通知 BLE |
| 当前状态 | 骨架已通，BLE 协议栈独立任务 |
| 入口 | `ble_module_start()` |
| 依赖 | `ble_phy_init()`、`gateway_on_state_change()` |

---

## 8. WiFi / 无线 / 第三方模块

| 模块 | 文件 | 状态 |
|------|------|------|
| WiFi | `src/modules/wifi_module.c/.h` | TODO |
| 无线（米家/涂鸦） | `src/modules/wireless_module.c/.h` | TODO |
| 第三方 485 | `src/modules/third_party_module.c/.h` | TODO |

这些模块都预留了标准 `module_ops_t` / `event_handler_t` 结构，后续按 AC 模块同样方式填充即可。

---

## 9. 物理层

### 9.1 AC 物理层

| 项目 | 内容 |
|------|------|
| 文件 | `src/phy/ac_phy.c`、`ac_phy_rs485.c`、`ac_phy_uart.c` |
| 职责 | 创建 encoder/decoder/sender/receiver/rs485 并注入模块 |
| AC 当前物理层 | `AC_PHY_RS485` |
| UART | 从 `bsp_ac_phy_cfg()` 读取，当前 UART0 |
| RS485 | DE=PB5, RE=PB6，从 BSP 读取 |

### 9.2 Debug 物理层

| 项目 | 内容 |
|------|------|
| 文件 | `src/phy/debug_phy.c` |
| UART | 固定 UART1 115200 |
| 帧缓冲 | CMD 512 / NORM 256 |

### 9.3 BLE 物理层

| 项目 | 内容 |
|------|------|
| 文件 | `src/phy/ble_phy.c` |
| 职责 | 创建 BLE TMOS 任务 |
| 中断通知 | `ble_phy_notify_from_isr()` |

---

## 10. HAL 层

### 10.1 bus

| 文件 | 职责 |
|------|------|
| `src/hal/bus/bus.c/.h` | 半双工总线状态、方向控制、gap |

关键状态：
```c
busy       总线忙
tx_active  本机正在发送
gap_until  帧间静默截止
need_tx_complete  RS485 需等 TX 完全结束
set_dir    方向回调
```

### 10.2 frame / sender / receiver

| 文件 | 职责 |
|------|------|
| `sender.c` | 帧发送、双 ring、CMD/NORM 优先级 |
| `sender_complete_poll.c` | 软定时器轮询 TX_COMPLETE |
| `receiver.c` | 帧接收队列 |
| `receiver_timeout.c` | 帧间隙超时组帧 |

### 10.3 uart

| 文件 | 职责 |
|------|------|
| `uart_ch579.c` | CH579 UART 配置/中断/读写 |
| `uart_instance.c` | uart0~uart3 实例 |

### 10.4 rs485

| 文件 | 职责 |
|------|------|
| `rs485.c` | 抽象接口 |
| `rs485_ch579.c` | DE/RE 方向控制实现 |

### 10.5 timer

| 文件 | 职责 |
|------|------|
| `timer.c/.h` | 抽象接口 |
| `timer_ch579.c` | CH579 硬件定时器 |
| `timer_soft.c` | 多实例软定时器 |
| `timer_hw.c` | 硬件定时器统一入口 |
| `timer_instance.c` | timer0~timer3 实例 |

### 10.6 gpio / led / queue

| 文件 | 职责 |
|------|------|
| `gpio*` | GPIO 抽象与 CH579 实现 |
| `led*` | LED |
| `ring.c` / `frame_queue.c` | 环形缓冲/帧队列 |

---

## 11. BSP 板级

### 11.1 文件

| 文件 | 职责 |
|------|------|
| `src/bsp/bsp.h` | 板级抽象 |
| `src/bsp/bsp_a07s.c/.h` | A07S 实现 |
| `src/bsp/bsp_board.c` | 板级选择 |

### 11.2 板级配置集中点

```c
bsp_a07s.c
├── ch579_init()           GPIO 初始化
├── ch579_ac_select()      品牌电路切换
├── ch579_rs485_enable()   485 电路
└── ch579_get_ac_phy_cfg() AC 物理层引脚/串口配置
```

当前 `bsp_ac_phy_cfg_t`：

```c
uart_id       = 0
rs485_port    = 1   // GPIOB
de_pin        = GPIO_Pin_5
re_pin        = GPIO_Pin_6
rs485_invert  = 0
```

---

## 12. 修改入口速查表

| 想做什么 | 去哪里改 |
|----------|----------|
| 改 AC 串口/DE/RE 引脚 | `src/bsp/bsp_a07s.c` |
| 改 AC 波特率/帧超时 | 对应品牌 `ac_xxx_cfg` 的 phy_cfg |
| 新增 AC 品牌协议 | `src/brands/ac_xxx.c` + `ac_brand.h` 注册表 |
| 改 AC 轮询周期 | 品牌 `state_machine` / `poll_period_ms` |
| 改扫描/锁定逻辑 | `src/modules/ac_module.c` |
| 改日志命令 | `src/modules/debug_module.c` |
| 改状态同步/观察者 | `src/core/gateway_device.c` |
| 改模块任务/队列大小 | `src/core/module.h` / `module.c` |
| 改发送策略 | `src/hal/frame/sender*.c` |
| 改接收组帧 | `src/hal/frame/receiver*.c` |
| 改 UART 中断 | `src/hal/uart/uart_ch579.c` |
| 改 RS485 方向 | `src/hal/rs485/rs485_ch579.c` |
| 改 FreeRTOS 配置 | `lib/FreeRTOS/FreeRTOSConfig.h` |

---

## 13. 事件流向快速记忆

```text
周期事件  → send_task → AC_EV_POLL → 品牌 state_machine → 发帧
收到应答  → receive_task → rx_parse → 锁定/更新状态 → EVENT_AC_RX → send_task
网关命令  → send_task → AC_EV_STATE_SYNC → 品牌 state_machine → 发控制帧
状态变化  → gateway_device → 观察者 + 同步其他模块
```
