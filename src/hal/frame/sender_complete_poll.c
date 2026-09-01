/**
 * sender_complete_poll.c —— 发送完成策略：软件定时器轮询
 *
 * 适用于没有 TX 完成中断的 MCU（如 CH579），
 * 通过软件定时器周期读取发送完成状态位（TX_ALL_EMP / TC）。
 */
#include "sender.h"

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
    if (!tx) return;

    if (!tx->complete_timer) {
        tx->complete_timer = (void *)xTimerCreate(
            "txcmp", pdMS_TO_TICKS(1), pdFALSE, tx, poll_timer_cb);
    }
    if (tx->complete_timer)
        xTimerStart((TimerHandle_t)tx->complete_timer, 0);
}

static void poll_stop(sender_t *tx)
{
    if (tx && tx->complete_timer)
        xTimerStop((TimerHandle_t)tx->complete_timer, 0);
}
#endif

const sender_complete_ops_t sender_complete_poll_ops = {
    .start = poll_start,
    .stop  = poll_stop,
};
