#include "receiver.h"

void receiver_init(receiver_t *rx) {
    if (!rx || !rx->ops) return;
    rx->ops->init(rx);
}

void receiver_reset(receiver_t *rx) {
    if (!rx) return;
    rx->frames.head = rx->frames.tail = rx->frames.count = 0;
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

uint8_t receiver_push_frame(receiver_t *rx, uint16_t len)
{
    if (!rx || len == 0)
        return 1;

    if (rx->frames.count >= RX_FRAME_QUEUE_LEN) {
        rx->frame_drop_cnt++;
        return 1;
    }

    rx->frames.lens[rx->frames.tail] = len;
    rx->frames.tail = (uint8_t)((rx->frames.tail + 1) % RX_FRAME_QUEUE_LEN);
    rx->frames.count++;
    return 0;
}

uint8_t receiver_has_frame(const receiver_t *rx)
{
    return rx && rx->frames.count > 0;
}

uint16_t receiver_read_frame(receiver_t *rx, uint8_t *buf, uint16_t max) {
    uint16_t len, cnt;

    if (!rx || !buf || !max || rx->frames.count == 0)
        return 0;

    len = rx->frames.lens[rx->frames.head];
    cnt = ring_count(&rx->ring);
    if (len > cnt) len = cnt;   /* 防御：ring 数据不足时按实际可读 */
    if (len > max) len = max;
    if (!len)
        return 0;

    ring_read(&rx->ring, buf, len);
    rx->frames.head = (uint8_t)((rx->frames.head + 1) % RX_FRAME_QUEUE_LEN);
    rx->frames.count--;
    return len;
}

uint16_t receiver_frame_drop_count(const receiver_t *rx)
{
    return rx ? rx->frame_drop_cnt : 0;
}

void receiver_set_callback(receiver_t *rx, frame_finish_callback cb) {
    if (!rx) return;
    rx->on_frame_finish = cb;
}

void receiver_set_bus(receiver_t *rx, bus_t *bus) {
    if (!rx) return;
    rx->bus = bus;
}
