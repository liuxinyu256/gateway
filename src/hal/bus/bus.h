#ifndef BUS_H
#define BUS_H
#include <stdint.h>

/**
 * bus —— 半双工总线抽象
 *
 * 只负责：
 *   - 总线忙/闲状态
 *   - 帧间静默间隔
 *
 * RS485 方向控制（DE）不属于 bus，由物理层装配时注入 sender/receiver。
 */

typedef struct {
    volatile uint8_t  busy;
    uint16_t          gap_ms;
    volatile uint32_t gap_until;
} bus_t;

void bus_init(bus_t *la, uint32_t baudrate);
void bus_mark_busy(bus_t *la);
void bus_mark_idle(bus_t *la);
void bus_mark_rx_busy(bus_t *la);     /* 接收侧占用总线 */
void bus_on_rx_complete(bus_t *la);   /* 接收完成，释放总线并进入 gap */
void bus_on_thr_empty(bus_t *la);     /* sender 发送队列空时上报 */
void bus_on_tx_complete(bus_t *la);   /* UART TX 完成中断里调用 */
int  bus_is_idle(const bus_t *la);

#endif
