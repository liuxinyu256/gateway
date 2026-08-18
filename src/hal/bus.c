/**
 * bus.c —— 半双工总线抽象
 *
 * 半双工总线上发送和接收共用一条线，帧与帧之间必须保留静默间隔。
 *
 * 空闲判定流程：
 *   发送完最后一字节
 *      ↓
 *   bus_mark_idle()
 *      ↓
 *   gap_until = now + gap_ms
 *      ↓
 *   （等待 gap_ms 毫秒）
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

void bus_mark_busy(bus_t *la) {
    la->busy = 1;
}

/* 发送完成：标记总线进入静默等待期 */
void bus_mark_idle(bus_t *la) {
    la->busy = 0;
#ifdef FAKE_FREERTOS
    la->gap_until = xTaskGetTickCount() + pdMS_TO_TICKS(la->gap_ms);
#else
    /* 该函数可能在 UART THR_EMPTY ISR 中调用，必须用 FromISR 版本 */
    la->gap_until = xTaskGetTickCountFromISR() + pdMS_TO_TICKS(la->gap_ms);
#endif
}

/* 总线空闲 = 不忙 且 已经过了静默间隔 */
int bus_is_idle(const bus_t *la) {
    return !la->busy && xTaskGetTickCount() >= la->gap_until;
}
