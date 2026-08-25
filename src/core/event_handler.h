#ifndef EVENT_HANDLER_H
#define EVENT_HANDLER_H
#include <stdint.h>

typedef enum {
    EVENT_PERIODIC_SEND,
    EVENT_RX_FRAME,
    EVENT_CONTROL_CMD,
    EVENT_NEED_ACK,
    EVENT_SCAN_AC,
    EVENT_TIMEOUT,
    EVENT_BUS_IDLE,
} event_type_t;

typedef struct {
    event_type_t type;
    uint8_t     *data;
    uint16_t     len;
    uint8_t      cmd_val;
    uint8_t      cmd_arg;
} event_t;

typedef struct {
    void (*on_activate)     (void *ctx);
    void (*on_periodic_send)(void *ctx);
    int  (*on_rx_frame)     (void *ctx, uint8_t *data, uint16_t len);
    void (*on_control_cmd)  (void *ctx, uint8_t cmd, uint8_t val);
    void (*on_need_ack)     (void *ctx);
    void (*on_scan)         (void *ctx);
    void (*on_timeout)      (void *ctx);
} event_handler_t;

#endif
