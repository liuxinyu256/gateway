#ifndef SENDER_H
#define SENDER_H
#include <stdint.h>

typedef struct sender sender_t;
typedef void (*sender_done_cb)(void *ctx);

typedef struct sender_ops {
    uint8_t (*init)(sender_t *tx, const void *cfg);

    uint8_t (*send_cmd)(sender_t *tx,
                        const uint8_t *frame, uint16_t len);
    uint8_t (*send)(sender_t *tx,
                    const uint8_t *frame, uint16_t len);
    uint8_t (*has_pending)(const sender_t *tx);
    uint8_t (*is_wait_tx_complete)(const sender_t *tx);

    void (*pump)(sender_t *tx);            /* 唯一“取下一帧并启动”的入口 */
    void (*on_thr_empty)(sender_t *tx);    /* UART THR_EMPTY ISR 调用 */
    void (*on_tx_complete)(sender_t *tx);  /* UART TX_COMPLETE ISR 调用 */
    void (*poll_tx_complete)(sender_t *tx);/* 任务上下文轮询 TX_COMPLETE */
    void (*uart_isr)(sender_t *tx);        /* UART ISR 统一入口 */

    void (*set_done_callback)(sender_t *tx,
                              void (*cb)(void *ctx), void *ctx);
    void (*set_wait_tx_complete_callback)(sender_t *tx,
                              void (*cb)(void *ctx), void *ctx);
} sender_ops_t;

struct sender {
    const sender_ops_t *ops;
};

uint8_t sender_init(sender_t *tx, const void *cfg);
uint8_t sender_send_cmd(sender_t *tx,
                        const uint8_t *frame, uint16_t len);
uint8_t sender_send(sender_t *tx,
                    const uint8_t *frame, uint16_t len);
uint8_t sender_has_pending(const sender_t *tx);
uint8_t sender_is_wait_tx_complete(const sender_t *tx);

void    sender_pump(sender_t *tx);
void    sender_on_thr_empty(sender_t *tx);
void    sender_on_tx_complete(sender_t *tx);
void    sender_poll_tx_complete(sender_t *tx);
void    sender_uart_isr(sender_t *tx);

void    sender_set_done_callback(sender_t *tx,
                                 void (*cb)(void *ctx), void *ctx);
void    sender_set_wait_tx_complete_callback(sender_t *tx,
                                 void (*cb)(void *ctx), void *ctx);

#endif
