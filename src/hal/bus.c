/**
 * bus.c —— 半双工总线抽象
 *
 * 半双工总线上发送和接收共用一条线，帧与帧之间必须保留静默间隔。
 *
 * 空闲判定流程：
 *   发送完最后一字节
 *      ↓
 *   sender 上报 bus_on_thr_empty()
 *      ↓
 *   非 RS485：直接 bus_mark_idle()
 *   RS485：等待 UART TX 完成
 *      ↓
 *   bus_on_tx_complete()
 *      ↓
 *   bus_mark_idle() → 释放 DE + gap_until = now + gap_ms
 *      ↓
 *   bus_is_idle() 返回 true
 *      ↓
 *   可以发下一帧
 */
#include "bus.h"
#ifdef FAKE_FREERTOS
#include "fake_freertos.h"
#else
#include "FreeRTOS.h"
#endif
#include <string.h>

void bus_init(bus_t *la, uint32_t baudrate) {
    memset(la, 0, sizeof(*la));
    la->gap_ms = (uint16_t)(35000UL / baudrate);
    if (la->gap_ms < 1)  la->gap_ms = 1;
    if (la->gap_ms > 10) la->gap_ms = 10;
}

void bus_set_rs485_enable(bus_t *la, uint8_t enable)
{
    if (!la) return;
    la->rs485_enable = enable ? 1 : 0;
}

void bus_set_dir_callback(bus_t *la, bus_dir_cb cb, void *ctx)
{
    if (!la) return;
    la->set_dir = cb;
    la->dir_ctx = ctx;
}

void bus_mark_busy(bus_t *la) {
    if (!la) return;
    la->busy = 1;
    if (la->set_dir)
        la->set_dir(1, la->dir_ctx);   /* RS485: 进入发送方向 */
}

/* 真正的空闲：释放 DE 并开始静默计时 */
void bus_mark_idle(bus_t *la) {
    if (!la) return;
    la->busy = 0;
    if (la->set_dir)
        la->set_dir(0, la->dir_ctx);   /* RS485: 释放 DE，转回接收 */
#ifdef FAKE_FREERTOS
    la->gap_until = xTaskGetTickCount() + pdMS_TO_TICKS(la->gap_ms);
#else
    /* 该函数可能在 UART 中断中调用，必须用 FromISR 版本 */
    la->gap_until = xTaskGetTickCountFromISR() + pdMS_TO_TICKS(la->gap_ms);
#endif
}

/* 接收侧占用总线：标记忙，但保持接收方向 */
void bus_mark_rx_busy(bus_t *la) {
    if (!la) return;
    la->busy = 1;
    if (la->set_dir)
        la->set_dir(0, la->dir_ctx);   /* RS485: 保持接收方向 */
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

    if (la->rs485_enable) {
        /* RS485: 等 UART TX 完成再释放 DE */
        return;
    }

    bus_mark_idle(la);
}

/* UART TX 完成中断里调用：只有真正发完才能换向 */
void bus_on_tx_complete(bus_t *la)
{
    if (!la) return;
    bus_mark_idle(la);
}

int bus_is_idle(const bus_t *la) {
    return !la->busy && xTaskGetTickCount() >= la->gap_until;
}
