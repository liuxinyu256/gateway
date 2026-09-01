/**
 * sender_complete_poll.c —— 发送完成策略：软件定时器轮询
 *
 * 适用于没有 TX 完成中断的 MCU（如 CH579），
 * 通过软件定时器周期读取发送完成状态位（TX_ALL_EMP / TC）。
 */
#include "sender_complete_poll.h"
#include <stddef.h>

#ifdef FAKE_FREERTOS
static void poll_start(sender_t *tx) { (void)tx; }
static void poll_stop(sender_t *tx)  { (void)tx; }
#else
#include "FreeRTOS.h"
#include "timers.h"

static void poll_timer_cb(TimerHandle_t t)
{
    sender_t *tx = (sender_t *)pvTimerGetTimerID(t);
    if (tx && sender_poll_tx_complete(tx))
        xTimerStart(t, 0);
}

static void poll_start(sender_t *tx)
{
    sender_poll_t *ptx = (sender_poll_t *)tx;
    if (!ptx) return;

    if (!ptx->timer) {
        ptx->timer = (void *)xTimerCreate(
            "txcmp", pdMS_TO_TICKS(1), pdFALSE, tx, poll_timer_cb);
    }
    if (ptx->timer)
        xTimerStart((TimerHandle_t)ptx->timer, 0);
}

static void poll_stop(sender_t *tx)
{
    sender_poll_t *ptx = (sender_poll_t *)tx;
    if (ptx && ptx->timer)
        xTimerStop((TimerHandle_t)ptx->timer, 0);
}
#endif

const sender_complete_ops_t sender_complete_poll_ops = {
    .start = poll_start,
    .stop  = poll_stop,
};

uint8_t sender_poll_init(sender_poll_t *tx, const sender_cfg_t *cfg)
{
    sender_cfg_t cfg2;

    if (!tx || !cfg)
        return 1;

    tx->timer = NULL;
    cfg2 = *cfg;
    cfg2.complete_ops = &sender_complete_poll_ops;
    return sender_init(&tx->base, &cfg2);
}
