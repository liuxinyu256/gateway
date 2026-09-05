#ifndef EVENT_HANDLER_H
#define EVENT_HANDLER_H
#include <stdint.h>

typedef struct gateway_state gateway_state_t;

typedef enum {
    EVENT_PERIODIC_SEND,
    EVENT_RX_FRAME,
    EVENT_GATEWAY_CMD,
    EVENT_NEED_ACK,
    EVENT_SCAN_AC,
    EVENT_TICK,
    EVENT_BUS_IDLE,
    EVENT_AC_RX,      /* AC 模块内部：接收侧解析后通知发送状态机 */
    EVENT_DEBUG_TX,   /* Debug 模块内部：日志/回显投递到 send_task 发送 */
    EVENT_SEND_FRAME, /* 测试/通用：把一帧投递给 AC send_task 发送 */
} event_type_t;

typedef struct {
    event_type_t type;
    uint16_t     len;
    uint8_t      cmd_val;
    uint8_t      cmd_arg;
    const gateway_state_t *state;   /* cmd 事件携带完整状态时使用 */
    const void            *data;    /* 通用事件携带外部数据指针（必须指向持久内存） */
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
