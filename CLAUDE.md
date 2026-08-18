# CLAUDE.md

This file provides guidance to Claude Code (claude.ai/code) when working with code in this repository.

## Project Overview

CH579 multi-channel communication gateway — protocol bridge + state hub. Bridges wireless modules (BLE/WiFi/Tuya) to HVAC equipment (RS485/HBS/X1X2) and wired control panels.

- **MCU**: WCH CH579, ARM Cortex-M0
- **RTOS**: FreeRTOS
- **Build**: CMake 3.14+, MinGW Makefiles (Windows), GCC
- **Simulation**: PC simulator via `sim/fake_freertos.h`

## Build Commands

```bash
mkdir build && cd build
cmake .. -G "MinGW Makefiles"
cmake --build .
ctest                           # Run unit tests
```

Build targets:
- `sim_app` — PC simulator (default, `-DPC_SIM=ON`)
- `gateway_ch579.elf` — Embedded target (`-DCH579=ON -DPC_SIM=OFF`)
- `test_receiver` — Unit tests for receiver

## Architecture

```
gateway_device (top-level state hub, observer chain)
├─ module_t[0]: HVAC RS485   → ac_brand_manager → ac_gree.c / ac_midea.c / ...
├─ module_t[1]: Wireless UART → wireless_module_mgr (Mi/Tuya)
├─ module_t[2]: 3rd-party 485 → third_party_mgr
├─ module_t[3]: Extension
└─ module_t[4]: WiFi UART     → wifi_mgr
```

### Per-Module Internals

Each `module_t` is a bidirectional communication pipe:

```
phy ─→ receiver ─→ bus ─→ rx_task(P4)    ← Frame receive path
phy ←─ sender   ←─ bus ←─ send_task(P3)   ← Frame transmit path
```

**Dual-task per module**: `rx_task` (priority 4) for frame parsing, `send_task` (priority 3) for frame transmission. ISR TX pipeline (THR_EMPTY interrupt chain) handles byte-by-byte UART output.

### Data Flow

```
RX: UART ISR → phy→forward_received_byte() → brand→on_rx_byte()
    → receiver_put_byte() → timer reload → inter-byte timeout
    → frame done callback → rx_task → brand→on_rx_frame()
    → gateway_state_update() → notify observers

TX: Timer/BLE/Cross-module → queue → send_task wakes
    → brand→on_periodic_send() → sender_send(frame, len)
    → UART THR_EMPTY ISR chain → byte-by-byte → idle
```

### Event Handler Contract

**All handlers must return immediately — no blocking, no waiting.**

```c
event_handler_t {
    on_rx_byte         // ISR: feed byte to receiver
    on_periodic_send   // send_task: query/heartbeat frames
    on_rx_frame        // rx_task: parse response, update state
    on_rx_isr          // ISR: urgent ACK (must reply within 1.5ms)
    on_control_cmd     // send_task: external control command
    on_need_ack        // send_task: protocol handshake
    on_scan            // send_task: power-on brand scan
    on_timeout         // send_task: retry or abandon
}
```

## Key Types

```c
gateway_device_t { state, modules[5], on_change[8] }  // Top-level state hub
module_t { phy, pkt, bus, rx_task, send_task, handler, handler_ctx }
brand_t { mod, state, timeout_at, retry, max_retry }   // HVAC state machine
```

## Directory Structure

```
src/
├── hal/           # Platform-independent HAL interfaces + implementations
│                  # ring, receiver, sender, bus
│                  # phy (uart1), phy_factory
├── core/          # Framework core
│                  # gateway_device, module, brand, event_handler, gateway.h
├── managers/      # Module managers
│                  # ac_brand_manager, wireless_module_mgr, third_party_mgr, wifi_mgr
└── brands/        # Brand protocols (state machines)
    └── ac_gree.c
app/               # Entry point (main.c, hvac_init.c)
bsp/CH579/         # CH579 BSP (StdPeriphDriver)
lib/               # FreeRTOS, CMSIS
sim/               # PC simulator (sim_app.c, fake_freertos.h)
test/              # Unit tests
```

## Platform Isolation

```
framework / manager / brand  ← no platform headers
         │
    PAL interface (phy.h)
         │
    Platform impl (_ch579.c)  ← selected at compile time
```

- `sim/fake_freertos.h` — FreeRTOS API stubs for PC simulation

## Naming Conventions

- `src/hal/` — Hardware Abstraction Layer (platform-independent interfaces)
- `src/core/` — Framework core (gateway device, module, bus, event system)
- `src/managers/` — Module managers (brand scanning, wireless, WiFi)
- `src/brands/` — Brand-specific HVAC protocol state machines
- `event_handler_t` — 8-callback event table per brand (all non-blocking)
