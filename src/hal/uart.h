#ifndef UART_H
#define UART_H
#include <stdint.h>

/**
 * uart —— 通用 UART 物理层接口
 *
 * 方式 B（Zephyr 风格）：
 *   - 驱动中断里只调用统一回调 irq_cb(u, user_data)
 *   - 上层在回调里通过 irq_rx_ready / irq_tx_ready / irq_tx_complete 判断状态
 *   - 真正读写：read / write
 */

typedef struct uart uart_t;
typedef void (*uart_irq_callback)(uart_t *u, void *user_data);

typedef struct {
    uint32_t baudrate;
    uint8_t  data_bits;   /* 5/6/7/8 */
    uint8_t  stop_bits;   /* 1/2 */
    uint8_t  parity;      /* 0=none, 1=odd, 2=even */
} uart_cfg_t;

typedef struct uart_ops {
    int  (*configure)(uart_t *u, const uart_cfg_t *cfg);
    int  (*read)(uart_t *u, uint8_t *byte);
    int  (*write)(uart_t *u, uint8_t byte);

    void (*irq_rx_enable)(uart_t *u);
    void (*irq_rx_disable)(uart_t *u);
    int  (*irq_rx_ready)(uart_t *u);

    void (*irq_tx_enable)(uart_t *u);
    void (*irq_tx_disable)(uart_t *u);
    int  (*irq_tx_ready)(uart_t *u);
    int  (*irq_tx_complete)(uart_t *u);

    void (*irq_callback_set)(uart_t *u, uart_irq_callback cb, void *user_data);
} uart_ops_t;

struct uart {
    const uart_ops_t *ops;
    void             *drv;

    uart_irq_callback irq_cb;
    void             *irq_user_data;
};

int  uart_configure(uart_t *u, const uart_cfg_t *cfg);
int  uart_read(uart_t *u, uint8_t *byte);
int  uart_write(uart_t *u, uint8_t byte);

void uart_irq_rx_enable(uart_t *u);
void uart_irq_rx_disable(uart_t *u);
int  uart_irq_rx_ready(uart_t *u);

void uart_irq_tx_enable(uart_t *u);
void uart_irq_tx_disable(uart_t *u);
int  uart_irq_tx_ready(uart_t *u);
int  uart_irq_tx_complete(uart_t *u);

void uart_irq_callback_set(uart_t *u, uart_irq_callback cb, void *user_data);

#endif
