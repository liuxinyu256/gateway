/**
 * bus.c —— 半双工总线抽象
 *
 * 半双工总线上发送和接收共用一条线，帧与帧之间必须保留静默间隔。
 *
 * 空闲判定流程：
 *   发送完最后一字节
 *      ↓
 *   不需要等 TX 完成：sender 调 bus_on_thr_empty() → bus_mark_idle()
 *   需要等 TX 完成（如 RS485）：sender 等 UART TX 完成后再调 bus_on_tx_complete()
 *      ↓
 *   bus_mark_idle() → 释放方向(回接收) + gap_until = now + gap_ms
 *      ↓
 *   bus_is_idle() 返回 true
 *      ↓
 *   可以发下一帧
 *
 * 方向控制是 bus 的职责，RS485 只是其中一种实现。
 */
#include "bus.h"
#ifdef FAKE_FREERTOS
#include "fake_freertos.h"
#else
#include "FreeRTOS.h"
#include "task.h"
#endif
#include <string.h>

void bus_init(bus_t *la, uint32_t baudrate) {
    memset(la, 0, sizeof(*la));
    la->gap_ms = (uint16_t)(35000UL / baudrate);
    if (la->gap_ms < 1)  la->gap_ms = 1;
    if (la->gap_ms > 10) la->gap_ms = 10;
}

void bus_set_dir_callback(bus_t *la, bus_dir_cb cb, void *ctx)
{
    if (!la) return;
    la->set_dir = cb;
    la->dir_ctx = ctx;
}

void bus_set_need_tx_complete(bus_t *la, uint8_t enable)
{
    if (!la) return;
    la->need_tx_complete = enable ? 1 : 0;
}

void bus_mark_busy(bus_t *la) {
    if (!la) return;
    la->busy      = 1;
    la->tx_active = 1;                 /* 本机正在发送 */
    if (la->set_dir)
        la->set_dir(1, la->dir_ctx);   /* 进入发送方向 */
}

/* 真正的空闲：释放方向(回接收)并开始静默计时 */
void bus_mark_idle(bus_t *la) {
    if (!la) return;
    la->busy      = 0;
    la->tx_active = 0;                 /* 发送/接收结束 */
    if (la->set_dir)
        la->set_dir(0, la->dir_ctx);   /* 释放方向，转回接收 */
#ifdef FAKE_FREERTOS
    la->gap_until = xTaskGetTickCount() + pdMS_TO_TICKS(la->gap_ms);
#else
    /* 该函数可能在 UART 中断中调用，必须用 FromISR 版本 */
    la->gap_until = xTaskGetTickCountFromISR() + pdMS_TO_TICKS(la->gap_ms);
#endif
}

/* 接收侧占用总线：标记忙，保持接收方向 */
void bus_mark_rx_busy(bus_t *la) {
    if (!la) return;
    if (la->tx_active)
        return;                        /* 发送中忽略回环/噪声，防止抢方向截断帧 */
    la->busy = 1;
    if (la->set_dir)
        la->set_dir(0, la->dir_ctx);   /* 保持/切回接收方向 */
}

/* 接收完成：释放总线并进入帧间静默 */
void bus_on_rx_complete(bus_t *la) {
    if (!la) return;
    bus_mark_idle(la);
}

/* sender 发送队列空时上报 */
void bus_on_thr_empty(bus_t *la)
{
    if (!la) return;

    if (la->need_tx_complete) {
        /* 方向控制（如 RS485）需等 UART TX 完成再换向 */
        return;
    }

    bus_mark_idle(la);
}

/* UART TX 完成中断/轮询里调用：真正发完，进入帧间静默并换向 */
void bus_on_tx_complete(bus_t *la)
{
    if (!la) return;
    bus_mark_idle(la);
}

int bus_is_idle(const bus_t *la) {
    return !la->busy && xTaskGetTickCount() >= la->gap_until;
}
