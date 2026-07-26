# 网关框架设计

## 1. 网关本质

**协议桥 + 状态中枢**。多路上行模块通过网关控制 HVAC，状态变更后同步回所有上行模块。

## 2. 层级结构

```
gateway_device          顶层 — 全局状态 + 观察者链
├─ module_t[0]          模块1 — HVAC RS485
│   └─ ac_brand_manager 品牌管理器
│       ├─ ac_gree.c    格力协议 (状态机)
│       └─ ac_xxx.c     美的/新风/地暖...
├─ module_t[1]          模块2 — 无线模组 (米家/涂鸦)
│   └─ wireless_module_mgr
├─ module_t[2]          模块3 — 第三方485
│   └─ third_party_mgr
├─ module_t[3]          模块4 — 拓展 (预留)
├─ module_t[4]          模块5 — WiFi模块
│   └─ wifi_mgr
└─ HAL                  phy / packetizer / frame_timer / ring
```

## 3. 事件驱动模型

**核心契约：所有 handler 立即返回，不做任何等待。**

```
事件源                    任务                    handler
定时器到期  ──→ send_task (P3) ──→ on_periodic_send()  → bus_send → 返回
外部控制    ──→ send_task (P3) ──→ on_control_cmd()    → bus_send → 返回
超时        ──→ send_task (P3) ──→ on_timeout()        → 重发/放弃 → 返回
帧完成      ──→ rx_task   (P4) ──→ on_rx_frame()       → 解析/更新状态 → 返回
紧急帧      ──→ ISR 上下文      ──→ on_rx_isr()         → 直接ACK → 返回
```

双任务 + ISR 发送管线：

```
rx_task   (P4)  TaskNotify → on_rx_frame → 返回
send_task (P3)  Queue → 组帧 → bus_send → 返回
TX管线    (ISR) bus_send首字节 → THR_EMPTY ISR链式发完
```

## 4. 数据流

```
控制流 (多源 → HVAC):
  无线模组 ─→ gateway_send_cmd() ─→ 模块1.send_task ─→ RS485 ─→ HVAC
  WiFi    ─→ gateway_send_cmd() ─┘
  BLE     ─→ gateway_send_cmd() ─┘

状态同步 (HVAC → 所有模块):
  HVAC应答 → 模块1.rx_task → gateway_state_update()
           → 遍历 on_change[]: ble_notify / wireless_sync / wifi_sync
```

## 5. 关键类型

```c
// 顶层
gateway_device_t { state, modules[5], on_change[8] }

// 模块 — 通信管道
module_t { phy, pkt, bus, rx_task(P4), send_task(P3), handler, handler_ctx }

// 品牌 — HVAC状态机
brand_t { mod, state, timeout_at, retry, max_retry }

// 事件表 — 8个回调
event_handler_t { on_rx_byte, on_periodic_send, on_rx_frame, on_rx_isr,
                  on_control_cmd, on_need_ack, on_scan, on_timeout }
```

## 6. 文件结构

```
src/
├── hal/                    HAL (平台无关接口)
│   ├── ring, packetizer, packetizer_timeout
│   ├── frame_timer, frame_timer_hw, frame_timer_sw
│   └── phy, phy_uart1
├── core/                   框架核心
│   ├── gateway_device      顶层状态中枢
│   ├── module              模块基类
│   ├── bus                 ISR发送管线
│   ├── event_handler      事件表
│   ├── brand               brand_t
│   └── gateway.h          统一入口
├── managers/               模块管理器
│   ├── ac_brand_manager    HVAC品牌管理(多品牌扫描)
│   ├── wireless_module_mgr 无线模组
│   ├── third_party_mgr     第三方485
│   └── wifi_mgr            WiFi模块
└── brands/                 品牌协议
    └── ac_gree.c           格力(状态机)
```

## 7. 平台隔离

```
framework / manager / brand ← 不含平台头文件
        │
   PAL 接口 (phy.h / frame_timer.h)
        │
  平台实现 (_ch579.c)  ← 编译时按平台选择
```
