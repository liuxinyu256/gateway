#ifndef BUS_H
#define BUS_H
#include <stdint.h>

/**
 * bus —— 半双工总线抽象
 *
 * 负责：
 *   - 总线忙/闲状态
 *   - 帧间静默间隔
 *   - 方向控制（RS485 只是方向控制的一种实现）
 *
 * 方向控制通过回调注入：bus 只依赖“1=发送 / 0=接收”，
 * 不关心具体是 DE 引脚、收发器使能还是其他机制。
 */

typedef void (*bus_dir_cb)(uint8_t tx, void *ctx);

typedef struct {
    volatile uint8_t  busy;
    uint16_t          gap_ms;
    volatile uint32_t gap_until;

    uint8_t           need_tx_complete; /* 1=方向控制需等 TX 完全结束后才能换向 */
    bus_dir_cb        set_dir;          /* 方向控制: 1=发送, 0=接收 */
    void             *dir_ctx;
} bus_t;

void bus_init(bus_t *la, uint32_t baudrate);
void bus_set_dir_callback(bus_t *la, bus_dir_cb cb, void *ctx);
void bus_set_need_tx_complete(bus_t *la, uint8_t enable);
void bus_mark_busy(bus_t *la);
void bus_mark_idle(bus_t *la);
void bus_mark_rx_busy(bus_t *la);     /* 接收侧占用总线（保持接收方向） */
void bus_on_rx_complete(bus_t *la);   /* 接收完成，释放总线并进入 gap */
void bus_on_thr_empty(bus_t *la);     /* sender 发送队列空时上报 */
void bus_on_tx_complete(bus_t *la);   /* UART TX 完成中断里调用 */
int  bus_is_idle(const bus_t *la);

#endif
