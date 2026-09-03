/**
 * receiver_timeout.c —— 超时策略接收器 (帧间隙超时 → 帧完成)
 *
 * 与发送侧“帧任务 + ring”对齐：
 *   - ring 存所有未读帧的字节
 *   - frames 队列只存每帧长度
 *   - 每完成一帧，把长度放入 frames 队列，再触发回调
 */
#include "receiver_timeout.h"
#include <string.h>

static void to_init(receiver_t *pkt)
{
    receiver_timeout_t *s = (receiver_timeout_t *)pkt;
    if (s->timer)
        timer_stop(s->timer);
    s->base.receiving = 0;
    pkt->frame_len   = 0;
    pkt->frames.head = pkt->frames.tail = pkt->frames.count = 0;
}

static void to_reset(receiver_t *pkt)
{
    receiver_timeout_t *s = (receiver_timeout_t *)pkt;
    if (s->timer)
        timer_stop(s->timer);
    if (s->base.receiving && pkt->bus)
        bus_on_rx_complete(pkt->bus);
    s->base.receiving = 0;
    pkt->frame_len   = 0;
    pkt->frames.head = pkt->frames.tail = pkt->frames.count = 0;
    ring_reset(&pkt->ring);
}

static void to_on_byte(receiver_t *pkt)
{
    receiver_timeout_t *s = (receiver_timeout_t *)pkt;
    if (!s->timer || !s->timeout_ticks)
        return;
    if (!s->base.receiving)
    {
        if (pkt->bus)
            bus_mark_rx_busy(pkt->bus);
        timer_init(s->timer);
        s->base.receiving = 1;
        pkt->frame_len = 1;          /* 当前帧第一个字节 */
    }
    else
    {
        pkt->frame_len++;            /* 当前帧继续累加 */
        timer_reset(s->timer);
    }
}

static const receiver_ops_t timeout_ops = {to_init, to_reset, to_on_byte};

/* 定时中断回调: 帧间隙超时 → 当前帧完成 */
static void on_timeout(void *ctx)
{
    receiver_timeout_t *s = (receiver_timeout_t *)ctx;
    uint16_t len;

    if (!s || !s->timer || !s->timeout_ticks)
        return;
    if (s->timer->counter < s->timeout_ticks)
        return;

    timer_stop(s->timer);
    len = s->base.frame_len;
    s->base.receiving = 0;

    if (len == 0)
        return;

    if (receiver_push_frame(&s->base, len) != 0) {
        /* 帧任务队列满：丢弃刚完成的这一帧（回退 ring 尾部） */
        ring_unwrite(&s->base.ring, len);
        if (s->base.bus)
            bus_on_rx_complete(s->base.bus);
        return;
    }

    if (s->base.bus)
        bus_on_rx_complete(s->base.bus);

    if (s->base.on_frame_finish)
        s->base.on_frame_finish(&s->base, len);
}

void receiver_timeout_init(receiver_timeout_t *self,
                           timer_t *timer,
                           uint16_t timeout_ticks,
                           frame_finish_callback cb,
                           uint8_t *ring_buf, uint16_t ring_size)
{
    memset(self, 0, sizeof(*self));
    self->base.ops = &timeout_ops;
    self->base.on_frame_finish = cb;
    ring_init(&self->base.ring, ring_buf, ring_size);
    self->timer = timer;
    self->timeout_ticks = timeout_ticks;

    if (timer)
        timer_set_callback(timer, on_timeout, self);
}
