#ifndef RECEIVER_H
#define RECEIVER_H
#include "ring.h"

typedef struct receiver receiver_t;

typedef void (*frame_finish_callback)(receiver_t *rx, uint16_t len);

typedef struct {
    void (*init)(receiver_t *rx);
    void (*reset)(receiver_t *rx);
    void (*on_byte)(receiver_t *rx);
} receiver_ops_t;

struct receiver {
    const receiver_ops_t  *ops;
    ring_t                 ring;
    frame_finish_callback  on_frame_finish;
    uint16_t               frame_len;   /* 当前已完成帧的长度 */
};

void     receiver_init(receiver_t *rx);
void     receiver_reset(receiver_t *rx);
uint8_t  receiver_put_byte(receiver_t *rx, uint8_t byte);
uint16_t receiver_read_frame(receiver_t *rx, uint8_t *buf, uint16_t max);
void     receiver_set_callback(receiver_t *rx, frame_finish_callback cb);
#endif
