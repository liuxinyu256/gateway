#ifndef SENDER_H
#define SENDER_H
#include "ring.h"
#include "phy.h"
#include "bus.h"

typedef struct {
    ring_t           ring;
    uint8_t         *buf;
    uint16_t         buf_size;
    phy_driver_t    *phy;
    bus_t  *bus;
    void           (*on_done)(void *ctx);
    void            *done_ctx;
    uint8_t          idle;
} sender_t;

void sender_init(sender_t *tx, phy_driver_t *phy,
                      bus_t *bus,
                      uint8_t *buf, uint16_t buf_size);
int  sender_send(sender_t *tx, const uint8_t *frame, uint16_t len);
void sender_on_thr_empty(sender_t *tx);
void sender_set_done_callback(sender_t *tx,
                                    void (*cb)(void *ctx), void *ctx);
#endif
