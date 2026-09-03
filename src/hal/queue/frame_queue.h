#ifndef FRAME_QUEUE_H
#define FRAME_QUEUE_H
#include <stdint.h>

#define TX_FRAME_MAX         128
#define TX_FRAME_QUEUE_LEN   2       /* 普通发送器：CMD/普通队列各 2 帧 */
#define TX_FRAME_QUEUE_LEN_DEBUG 6   /* Debug/日志发送器：更深，避免日志突发丢弃 */

typedef struct {
    uint8_t  data[TX_FRAME_MAX];
    uint16_t len;
} tx_frame_t;

/* 队列只保存元数据；具体帧缓冲数组由每个 sender 实例/模块自行提供 */
typedef struct {
    tx_frame_t *pool;
    uint8_t     capacity;

    volatile uint8_t head;
    volatile uint8_t tail;
    volatile uint8_t count;

    volatile uint16_t drop_cnt;   /* 队列满导致整帧丢弃的计数 */
} frame_queue_t;

void     frame_queue_init(frame_queue_t *q,
                          tx_frame_t *pool, uint8_t capacity);
uint8_t  frame_queue_push(frame_queue_t *q,
                          const uint8_t *data, uint16_t len);
uint8_t  frame_queue_pop(frame_queue_t *q, tx_frame_t *out);
uint8_t  frame_queue_empty(const frame_queue_t *q);
uint16_t frame_queue_drop_count(const frame_queue_t *q);

#endif
