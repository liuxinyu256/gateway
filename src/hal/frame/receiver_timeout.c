/**
 * receiver_timeout.c —— 超时策略接收器 (帧间隙超时 → 帧完成)
 * receiver 子类: 定时器由程序员声明并注入 (占用哪个由程序员决定),
 * 每字节经定时器接口重新计时, 定时中断回调判定计数值到阈值 → 帧完成
 * (回调在定时中断上下文, 须轻量)
 */
#include "receiver_timeout.h"
#include <string.h>

static void to_init(receiver_t *pkt)
{
    receiver_timeout_t *s = (receiver_timeout_t *)pkt;
    if (s->timer)
        timer_stop(s->timer); /* 重新初始化: 停止旧定时器 */
    s->base.receiving = 0;     /* ring 由 init 初始化 */
    pkt->frame_len   = 0;
}

static void to_reset(receiver_t *pkt)
{
    receiver_timeout_t *s = (receiver_timeout_t *)pkt;
    if (s->timer)
        timer_stop(s->timer); /* 放弃当前帧, 关闭定时器 */
    if (s->base.receiving && pkt->bus)
        bus_on_rx_complete(pkt->bus);
    s->base.receiving = 0;
    pkt->frame_len   = 0;
    ring_reset(&pkt->ring);
}

static void to_on_byte(receiver_t *pkt)
{
    receiver_timeout_t *s = (receiver_timeout_t *)pkt;
    if (!s->timer || !s->timeout_ticks)
        return; /* 未绑定定时器或超时禁用: 纯缓冲 */
    if (!s->base.receiving)
    {
        if (pkt->bus)
            bus_mark_rx_busy(pkt->bus); /* 首字节: 接收占用总线，bus 保持接收方向 */
        timer_init(s->timer);           /* 首字节: 开启定时中断 */
        s->base.receiving = 1;
    }
    else
    {
        timer_reset(s->timer); /* 后续字节: 重新计时 */
    }
}

static const receiver_ops_t timeout_ops = {to_init, to_reset, to_on_byte};

/* 定时中断回调: 计数值到阈值 → 帧完成 */
static void on_timeout(void *ctx)
{
    receiver_timeout_t *s = (receiver_timeout_t *)ctx;
    if (!s || !s->timer || !s->timeout_ticks)
        return; /* 超时禁用或未绑定: 不触发帧完成 */
    if (s->timer->counter < s->timeout_ticks)
        return;
    timer_stop(s->timer); /* 一帧完成, 等下一帧首字节 */
    s->base.frame_len = ring_count(&s->base.ring);
    if (s->base.bus)
        bus_on_rx_complete(s->base.bus); /* 接收完成: 总线先进入空闲/gap */
    if (s->base.on_frame_finish)
        s->base.on_frame_finish(&s->base, s->base.frame_len);
    s->base.receiving = 0;
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
