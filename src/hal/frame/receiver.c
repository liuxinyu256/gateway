#include "receiver.h"

void receiver_init(receiver_t *rx) {
    if (!rx || !rx->ops) return;
    rx->ops->init(rx);
}

void receiver_reset(receiver_t *rx) {
    if (!rx) return;
    rx->frame_len = 0;
    if (rx->ops && rx->ops->reset) {
        rx->ops->reset(rx);
    } else {
        ring_reset(&rx->ring);
    }
}

uint8_t receiver_put_byte(receiver_t *rx, uint8_t byte) {
    if (!rx || !rx->ops) return 1;
    if (ring_put(&rx->ring, byte)) return 1;
    if (rx->ops->on_byte) rx->ops->on_byte(rx);
    return 0;
}

uint16_t receiver_read_frame(receiver_t *rx, uint8_t *buf, uint16_t max) {
    if (!rx || !buf || !max) return 0;

    uint16_t n = rx->frame_len;
    uint16_t cnt = ring_count(&rx->ring);
    if (n > cnt) n = cnt;   /* 防止 frame_len 超过实际数据 */
    if (n > max) n = max;
    if (!n) return 0;

    ring_peek(&rx->ring, buf, n);
    ring_skip(&rx->ring, n);
    rx->frame_len = 0;      /* 帧已读走 */
    return n;
}

void receiver_set_callback(receiver_t *rx, frame_finish_callback cb) {
    if (!rx) return;
    rx->on_frame_finish = cb;
}

void receiver_set_bus(receiver_t *rx, bus_t *bus) {
    if (!rx) return;
    rx->bus = bus;
}

void receiver_set_rs485(receiver_t *rx, rs485_t *rs) {
    if (!rx) return;
    rx->rs485 = rs;
}
