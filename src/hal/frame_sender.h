#ifndef FRAME_SENDER_H
#define FRAME_SENDER_H
#include "sender.h"
#include "frame_queue.h"
#include "encoder.h"
#include "bus.h"

/* sender_t 的具体实现：帧级队列 + 编码器 */
typedef struct {
    sender_t base;

    frame_queue_t cmd_q;      /* CMD 帧：优先发 */
    frame_queue_t norm_q;     /* 普通帧 */

    encoder_t    *encoder;
    bus_t        *bus;

    tx_frame_t    current;    /* 当前正在发送的帧 */
    uint16_t      current_pos;

    volatile uint8_t sending;          /* 1 = 当前有帧在发 */
    volatile uint8_t wait_tx_complete; /* RS485: 等最后一位上总线 */

    sender_done_cb on_done;
    void          *done_ctx;

    void (*on_wait_tx_complete)(void *ctx);
    void  *wait_ctx;
} frame_sender_t;

typedef struct {
    encoder_t *encoder;
    bus_t     *bus;
} frame_sender_cfg_t;

uint8_t frame_sender_init(frame_sender_t *tx,
                          const frame_sender_cfg_t *cfg);

#endif
