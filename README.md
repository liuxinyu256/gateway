# 网关 (Gateway)

CH579 多通道通信网关 —— 协议桥 + 状态中枢。

## 架构

```
gateway_device (顶层, 持有全局状态)
├─ module_t[0]: HVAC RS485   (多品牌: 格力/美的/新风/地暖)
├─ module_t[1]: 无线模块 UART (米家WiFi/蓝牙/涂鸦)
├─ module_t[2]: 第三方 485
├─ module_t[3]: 拓展接口
└─ module_t[4]: WiFi 模块 UART
```

每个模块 = phy + packetizer + bus(ISR发送管线) + rx_task(P4) + send_task(P3)

## 构建

```bash
mkdir build && cd build
cmake .. -G "MinGW Makefiles"
cmake --build .
ctest
```

## 文件结构

```
src/hal/      硬件抽象层 (ring, packetizer, frame_timer, phy)
src/core/     框架核心 (gateway_device, module, bus, brand_manager, event_handler)
src/brands/   品牌协议 (ac_gree.c ...)
sim/          PC模拟器
test/         单元测试
```
