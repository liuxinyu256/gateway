/**
 * transmitter.c —— 发送组帧器 (TX 配对 packetizer)
 * 接帧 → ring → ISR 链发 → 完成回调
 */
#include "sender.h"
#include <string.h>

void sender_init(sender_t *tx, void (*write_byte)(uint8_t byte),
                      bus_t *bus,
                      uint8_t *buf, uint16_t buf_size) {
    memset(tx, 0, sizeof(*tx));
    tx->write_byte = write_byte;
    tx->bus = bus;
    tx->buf = buf;
    tx->buf_size = buf_size;
    tx->idle = 1;
    ring_init(&tx->ring, buf, buf_size);
}

int sender_send(sender_t *tx, const uint8_t *frame, uint16_t len) {
    if (!tx || !frame || !len) return -1;

    int was_empty = ring_empty(&tx->ring);
    uint16_t wrote = ring_write(&tx->ring, frame, len);
    if (wrote != len) return -1;

    if (was_empty && tx->idle && bus_is_idle(tx->bus)) {
        tx->idle = 0;
        bus_mark_busy(tx->bus);
        uint8_t byte;
        ring_get(&tx->ring, &byte);
        if (tx->write_byte) tx->write_byte(byte);
    }
    return 0;
}

void sender_on_thr_empty(sender_t *tx) {
    if (!tx) return;

    if (!ring_empty(&tx->ring)) {
        uint8_t byte;
        ring_get(&tx->ring, &byte);
        if (tx->write_byte) tx->write_byte(byte);
    } else {
        tx->idle = 1;
        if (tx->bus)
            bus_on_thr_empty(tx->bus);   /* 总线自己决定是否等 TX 完成 */
        if (tx->on_done) tx->on_done(tx->done_ctx);
    }
}

void sender_set_done_callback(sender_t *tx,
                                    void (*cb)(void *ctx), void *ctx) {
    if (!tx) return;
    tx->on_done  = cb;
    tx->done_ctx = ctx;
}
