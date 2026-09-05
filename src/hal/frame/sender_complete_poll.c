/**
 * sender_complete_poll.c —— 发送完成策略：软定时器轮询
 *
 * 所有 sender_poll 共享一个硬件定时器 tick（1ms），
 * 每个实例用 soft_timer_t 独立启停，互不影响。
 */
#include "sender_complete_poll.h"
#include "timer_soft.h"
#include <stddef.h>

#ifdef FAKE_FREERTOS
static void poll_start(sender_t *tx) { (void)tx; }
static void poll_stop(sender_t *tx)  { (void)tx; }
#else
static void poll_cb(void *ctx)
{
    sender_t *tx = (sender_t *)ctx;
    sender_poll_t *ptx = (sender_poll_t *)ctx;

    if (!tx || !ptx)
        return;

    /* 还在等 TX_COMPLETE：保持运行；完成/无需等待则停 */
    if (!sender_poll_tx_complete(tx))
        soft_timer_stop(&ptx->soft);
}

static void poll_start(sender_t *tx)
{
    sender_poll_t *ptx = (sender_poll_t *)tx;
    if (ptx)
        soft_timer_reset(&ptx->soft);   /* 只启动自己，不影响其他实例 */
}

static void poll_stop(sender_t *tx)
{
    sender_poll_t *ptx = (sender_poll_t *)tx;
    if (ptx)
        soft_timer_stop(&ptx->soft);
}
#endif

const sender_ops_t sender_poll_ops = {
    .start = poll_start,
    .stop  = poll_stop,
};

uint8_t sender_poll_init(sender_poll_t *tx, const sender_cfg_t *cfg)
{
    if (!tx || !cfg)
        return 1;

    if (sender_init(&tx->base, cfg) != 0)
        return 1;
    tx->base.ops = &sender_poll_ops;

#ifndef FAKE_FREERTOS
    /* 注册为共享硬件 tick 上的软定时器实例，默认停止，等 poll_start 启动 */
    soft_timer_init(&tx->soft);
    soft_timer_set_callback(&tx->soft, poll_cb, &tx->base);
    soft_timer_stop(&tx->soft);
#endif

    return 0;
}
