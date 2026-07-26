# 网关架构

## 层级结构

```
┌──────────────────────────────────────────────────────────────┐
│                      gateway_device                          │
│              gateway_state_t (唯一真相源)                     │
│              on_change[] (观察者广播链)                        │
├──────────────────────────────────────────────────────────────┤
│                                                              │
│  module_t[0]           module_t[1]         module_t[2..4]    │
│  HVAC / RS485          无线模组 / TTL       第三方 / WiFi     │
│                                                              │
│  ┌──────────────────┐ ┌───────────────┐ ┌────────────────┐  │
│  │ac_brand_manager  │ │wireless_mgr   │ │ third_party_mgr│  │
│  │                  │ │               │ │ wifi_mgr       │  │
│  │ ┌──────────────┐ │ │ 米家WiFi/BLE  │ │                │  │
│  │ │ ac_gree.c    │ │ │ 涂鸦          │ │                │  │
│  │ │ ac_midea.c   │ │ └───────────────┘ └────────────────┘  │
│  │ │ ac_lanshe.c  │ │                                       │
│  │ └──────────────┘ │                                       │
│  └──────────────────┘                                       │
│                                                              │
│  ===== 每个 module_t 内部 =====                               │
│                                                              │
│          ┌──── rx_task(P4) ────┐                             │
│          │                     │                             │
│    phy ──┤  receiver ── bus ── ├── sender ──── phy          │
│          │                     │                             │
│          └─ send_task(P3) ─────┘                             │
│                ↑  event queue                                │
│                                                              │
├──────────────────────────────────────────────────────────────┤
│                         HAL                                  │
│                                                              │
│  ┌─ 接口 ──────────────────────────────────────────────┐     │
│  │ ring.h   receiver.h  sender.h  bus.h  phy.h         │     │
│  │ receiver_timeout.h  frame_timer.h  phy_uart1.h      │     │
│  └─────────────────────────────────────────────────────┘     │
│  ┌─ 实现 ──────────────────────────────────────────────┐     │
│  │ ring.c  receiver.c  sender.c  bus.c                 │     │
│  │ receiver_timeout.c  phy_uart1.c  phy_factory.c      │     │
│  │ frame_timer_hw.c (CH579)  frame_timer_sw.c (PC)     │     │
│  └─────────────────────────────────────────────────────┘     │
│  ┌─ 平台 ──────────────────────────────────────────────┐     │
│  │ bsp/CH579/   lib/FreeRTOS/   lib/CMSIS/             │     │
│  │ sim/fake_freertos.h (PC模拟)                         │     │
│  └─────────────────────────────────────────────────────┘     │
└──────────────────────────────────────────────────────────────┘
```

## 数据流

```
RX:  UART ISR
       │
       ▼
     phy→forward_received_byte()
       │
       ▼
     brand→on_rx_byte()
       │
       ▼
     receiver_put_byte()          ← 字节入环 + 启/重计定时器
       │
       ▼
     ... 字节间超时 ...
       │
       ▼
     定时器 ISR → 帧完成回调
       │
       ▼
     fd_N → 紧急检查(on_rx_isr) / TaskNotify(rx_task)
       │
       ▼
     rx_task → receiver_read_frame()
       │
       ▼
     brand→on_rx_frame()         ← 解析帧, 状态机
       │
       ▼
     gateway_state_update()       ← 更新全局状态, 广播观察者

─────────────────────────────────────

TX:  Timer到期 / BLE命令 / 跨模块控制
       │
       ▼
     queue → send_task 被唤醒
       │
       ▼
     event_handler→on_periodic / on_control / on_timeout
       │
       ▼
     sender_send(frame, len)      ← 帧入环, 总线空闲发首字节
       │
       ▼
     UART THR_EMPTY ISR → sender_on_thr_empty
       │
       ▼
     逐字节 phy→write → 队列空 → idle
```

## 事件表

```
event_handler_t:
  .on_rx_byte        ISR: 字节喂接收器
  .on_periodic_send  send_task: 查询帧/心跳帧
  .on_rx_frame       rx_task: 解析应答
  .on_rx_isr         ISR: 紧急ACK (1.5ms内必回)
  .on_control_cmd    send_task: 外部控制命令
  .on_need_ack       send_task: 协议握手
  .on_scan           send_task: 上电扫描
  .on_timeout        send_task: 超时重试
```

## 品牌配置

```
brand_config_t:
  name                    "gree"
  device_type             DEV_AC
  event_handler_t *table  品牌事件表
  brand_phy_t phy:
    type                  PHY_RS485_8N1
    baudrate              9600
  receiver_timeout_ticks  5
```
