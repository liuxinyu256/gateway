/**
 * frame_timer.h —— 定时器基类 (C 风格继承 + 多态 ops 虚表)
 *
 * 子类:
 *   frame_timer_hw — 硬件定时器 (frame_timer_hw.c)
 *   frame_timer_sw — 软件定时器 (frame_timer_sw.c)
 */

#ifndef FRAME_TIMER_H
#define FRAME_TIMER_H

#include <stdint.h>

typedef struct frame_timer frame_timer_t;
typedef void (*timer_callback)(void *ctx);

typedef struct {
    void (*start)       (frame_timer_t *t);
    void (*restart)     (frame_timer_t *t);
    void (*stop)        (frame_timer_t *t);
    void (*set_callback)(frame_timer_t *t, timer_callback cb, void *ctx);
} frame_timer_ops_t;

struct frame_timer {
    const frame_timer_ops_t *ops;
    timer_callback           cb;
    void                    *ctx;
    uint32_t                 counter;
};

#endif
