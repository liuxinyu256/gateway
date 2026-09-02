#ifndef EVENT_HANDLER_H
#define EVENT_HANDLER_H
#include <stdint.h>
#include "gateway_device.h"

typedef enum {
    EVENT_PERIODIC_SEND,
    EVENT_RX_FRAME,
    EVENT_GATEWAY_CMD,
    EVENT_NEED_ACK,
    EVENT_SCAN_AC,
    EVENT_TICK,
    EVENT_BUS_IDLE,
} event_type_t;

typedef struct {
    event_type_t type;
    uint16_t     len;
    uint8_t      cmd_val;
    uint8_t      cmd_arg;
    const gateway_state_t *state;   /* cmd 事件携带完整状态时使用 */
} event_t;

typedef struct {
    void (*on_activate)     (void *ctx);
    void (*on_periodic_send)(void *ctx);
    int  (*on_rx_frame)     (void *ctx, uint8_t *data, uint16_t len);
    void (*on_gateway_cmd)  (void *ctx, uint8_t cmd, uint8_t val,
                             const gateway_state_t *state);
    void (*on_need_ack)     (void *ctx);
    void (*on_scan)         (void *ctx);
    void (*on_tick)         (void *ctx);
} event_handler_t;

#endif
