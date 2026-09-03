#ifndef RECEIVER_H
#define RECEIVER_H
#include "ring.h"
#include "bus.h"

#define RX_FRAME_QUEUE_LEN 8   /* 最多缓存 8 个已完成未读帧的长度 */

typedef struct receiver receiver_t;

typedef void (*frame_finish_callback)(receiver_t *rx, uint16_t len);

/* 接收帧任务队列：只记录每帧长度，与发送侧帧任务对齐 */
typedef struct {
    uint16_t lens[RX_FRAME_QUEUE_LEN];
    uint8_t  head;
    uint8_t  tail;
    uint8_t  count;
} rx_frame_queue_t;

typedef struct {
    void (*init)(receiver_t *rx);
    void (*reset)(receiver_t *rx);
    void (*on_byte)(receiver_t *rx);
} receiver_ops_t;

struct receiver {
    const receiver_ops_t  *ops;
    bus_t                 *bus;      /* 绑定的半双工总线（接收也维护忙/闲） */
    ring_t                 ring;
    rx_frame_queue_t       frames;   /* 已完成帧长度队列 */
    frame_finish_callback  on_frame_finish;
    volatile uint8_t       receiving;   /* 1 = 正在接收一帧 */
    volatile uint16_t      frame_len;   /* 当前正在接收的帧字节数 */
    volatile uint16_t      frame_drop_cnt;
};

void     receiver_init(receiver_t *rx);
void     receiver_reset(receiver_t *rx);
uint8_t  receiver_put_byte(receiver_t *rx, uint8_t byte);
uint8_t  receiver_push_frame(receiver_t *rx, uint16_t len); /* 帧完成入队 */
uint8_t  receiver_has_frame(const receiver_t *rx);
uint16_t receiver_read_frame(receiver_t *rx, uint8_t *buf, uint16_t max);
uint16_t receiver_frame_drop_count(const receiver_t *rx);
void     receiver_set_callback(receiver_t *rx, frame_finish_callback cb);
void     receiver_set_bus(receiver_t *rx, bus_t *bus);
#endif
