# 网关系统设计报告

## 1. 项目概述

本项目是一个通用多模块网关系统，用于连接多种上位控制通道（蓝牙、WiFi、RS485 等）与被控设备（如 HVAC/AC）。

当前使用：

- CH579 作为实机测试平台
- PC 模拟器（FAKE_FREERTOS）作为开发与验证平台

架构目标：

- 可移植：硬件差异收敛在 HAL 层
- 事件驱动：各层级通过队列解耦，不阻塞
- 模块化：模块只上报，决策在网关
- 可测试：PC 模拟器可闭环验证

## 2. 总体架构

```text
┌─────────────────────────────────────────────┐
│ app：初始化、中断入口                         │
├─────────────────────────────────────────────┤
│ core：模块框架、网关状态、事件分发             │
├─────────────────────────────────────────────┤
│ modules：AC、无线、WiFi、第三方 485           │
├─────────────────────────────────────────────┤
│ hal：uart / timer / bus / sender / receiver │
│      / encoder / decoder / frame_queue       │
├─────────────────────────────────────────────┤
│ sim：PC 模拟器（FAKE_FREERTOS）              │
└─────────────────────────────────────────────┘
```

## 3. 分层职责

### 3.1 HAL 层

HAL 提供统一硬件抽象，所有抽象通过指针注入：

- `uart_t`：UART 配置、读写、中断回调
- `timer_t`：定时器初始化、重置、停止
- `decoder_t`：物理信号 → 字节回调
- `encoder_t`：字节 → 物理输出
- `receiver_t`：字节 → 完整帧
- `sender_t`：完整帧 → 字节输出
- `bus_t`：半双工总线状态机

### 3.2 模块层

模块代表一个独立通信通道：

```c
module_t {
    uint8_t module_id;
    gateway_state_t state;
    bus_t bus;
    sender_t *sender;
    receiver_t *receiver;
    TaskHandle_t send_task;
    TaskHandle_t receive_task;
    QueueHandle_t send_queue;
    QueueHandle_t receive_queue;
    ...
}
```

模块只负责：

- 接收本通道数据
- 解析并更新自己的状态
- 上报状态变化给网关
- 响应网关下发的控制事件

模块之间不直接路由，不做跨模块决策。

### 3.3 网关层

网关维护所有模块的状态，并统一处理状态变化：

- 保存每个模块的完整状态
- 通过状态事件队列接收模块上报
- 在状态任务中统一通知外部观察者
- 提供控制指令下发入口

## 4. 接收侧设计

### 4.1 数据流

```text
物理输入 → decoder → receiver → 完整帧 → receive_queue → 接收任务
```

### 4.2 多封包策略

接收器为抽象基类，支持多种封包策略：

- 超时封包：`receiver_timeout_t`
- 包头包尾封包：可扩展
- IDLE 中断封包：可扩展

### 4.3 回调喂字节

接收器不持有解码器，通过回调方式接收字节：

```c
uart_decoder_attach_receiver(&dec, &rx.base);
// decoder_set_rx_callback(decoder, decoder_to_receiver, rx)
```

## 5. 发送侧设计

### 5.1 数据流

```text
发送任务组帧 → sender_send() → 帧队列 → EVENT_BUS_IDLE 触发 sender_pump() → encoder → 物理输出
```

### 5.2 发送器

发送器为具体类，不使用 vtable：

```c
sender_t {
    frame_queue_t cmd_q;    // CMD 帧优先
    frame_queue_t norm_q;   // 普通帧
    encoder_t *encoder;
    bus_t *bus;
    tx_frame_t current;
    uint16_t current_pos;
    volatile uint8_t sending;
    volatile uint8_t wait_tx_complete;
    ...
}
```

### 5.3 发送接口

```c
sender_init()
sender_send(frame, len, priority)
sender_pump()
sender_isr()
sender_poll_tx_complete()
sender_set_callbacks()
```

### 5.4 发送状态机

- `send` 只入队，不保证立即发送
- 帧等待发送时，由 `EVENT_BUS_IDLE` 触发发送任务调用 `sender_pump()`
- `sender_pump()` 内部再确认 bus 空闲，空闲才出队启动
- CMD 队列优先于普通队列
- 物理层差异通过 encoder 注入

### 5.5 为什么需要帧队列

发送侧是多生产者 + 异步总线：

- 多个控制/查询源可能同时投递
- bus 忙时必须整帧暂存
- 需要 CMD 优先级
- 发送任务不能被阻塞

因此必须使用帧级队列。

## 6. 状态同步机制

### 6.1 统一完整状态

```c
gateway_state_t {
    uint8_t power;
    uint8_t mode;
    uint8_t set_temp;
    uint8_t room_temp;
    uint8_t fan;
    uint8_t swing;
    uint8_t error_code;

    uint16_t mode_caps;
    uint16_t fan_caps;
    uint16_t swing_caps;
    uint8_t temp_min;
    uint8_t temp_max;
    uint8_t temp_step;
    uint16_t features;
}
```

所有模块共用该结构，不同协议只是完整状态的子集。

### 6.2 状态更新流程

```text
模块状态变化
  ↓
module_update_state()
  ↓
module_publish_state()
  ↓
gateway_module_state_update()
  ├─ 保存 module_states[id]
  ├─ pending 合并
  └─ 入状态事件队列
  ↓
状态任务 / gateway_poll_state_events()
  ├─ 清 pending
  ├─ 读最新状态
  └─ 通知观察者
```

### 6.3 观察者模式

两层观察者：

```text
模块（被观察者） → 网关（观察者1）
网关（被观察者） → BT/WiFi/485/App（观察者2）
```

## 7. 并发设计

### 7.1 每级队列化

```text
物理输入 → 字节 ring → decoder → 帧事件队列 → 接收任务
发送任务 → 帧队列 → 发送状态机
模块状态变化 → 状态事件队列 → 状态任务
```

### 7.2 关键并发点

- 帧队列：整帧入队/出队，bus 忙时不出队
- CMD 优先级：cmd_q + norm_q
- pending 合并：同一模块最多一个状态事件
- 原子操作：检查 + 标记 + 投递必须临界区保护
- 单消费者：帧队列出队只有 `sender_pump()`

### 7.3 丢帧计数

所有队列均提供丢帧计数，用于实机验证：

```c
send_queue_drop_cnt
receive_queue_drop_cnt
state_event_drop_cnt
frame_queue_drop_count(cmd_q)
frame_queue_drop_count(norm_q)
```

## 8. 设计原则

1. 模块只上报，不做决策
2. 模块之间不路由
3. 指针注入，模块不持有具体硬件实现
4. 事件驱动，不阻塞
5. 该抽象的地方抽象，不该抽象的地方砍掉
6. 队列深度先留参数，实机用丢帧计数验证
7. 架构与具体芯片解耦

## 9. 验证方式

- PC 模拟器：`sim_app` 闭环 smoke test
- CH579 实机：后续通过丢帧计数和日志验证

当前 PC 模拟器验证结果：

```text
[OK] closed-loop smoke test passed
```
