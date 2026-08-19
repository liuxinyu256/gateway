#ifndef BUS_H
#define BUS_H
#include <stdint.h>

/**
 * bus —— 半双工总线抽象
 *
 * 负责：
 *   - 总线忙/闲状态
 *   - 帧间静默间隔
 *   - RS485 方向控制（DE）
 *   - TX 完成后的换向时机
 */

typedef void (*bus_dir_cb)(uint8_t tx, void *ctx);

typedef struct {
    volatile uint8_t  busy;
    uint16_t          gap_ms;
    volatile uint32_t gap_until;

    uint8_t           rs485_enable;  /* 1=RS485 半双工 */
    bus_dir_cb        set_dir;       /* RS485 方向控制: 1=发送, 0=接收 */
    void             *dir_ctx;
} bus_t;

void bus_init(bus_t *la, uint32_t baudrate);
void bus_set_rs485_enable(bus_t *la, uint8_t enable);
void bus_set_dir_callback(bus_t *la, bus_dir_cb cb, void *ctx);
void bus_mark_busy(bus_t *la);
void bus_mark_idle(bus_t *la);
void bus_on_thr_empty(bus_t *la);     /* sender 发送队列空时上报 */
void bus_on_tx_complete(bus_t *la);   /* UART TX 完成中断里调用 */
int  bus_is_idle(const bus_t *la);

#endif
