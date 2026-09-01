/**
 * sender_complete_isr.c —— 发送完成策略：UART 中断直接完成
 *
 * 适用于有 TX 完成中断的 MCU（如 STM32 TC 中断）。
 * start/stop 用于使能/关闭发送完成中断，具体由平台 UART ISR 调用
 * sender_tx_complete_isr() 或 sender_isr() 完成检测。
 */
#include "sender_complete_isr.h"

static void isr_start(sender_t *tx)
{
    (void)tx;
    /* TODO: 使能发送完成中断（TCIE 等） */
}

static void isr_stop(sender_t *tx)
{
    (void)tx;
    /* TODO: 关闭发送完成中断 */
}

const sender_complete_ops_t sender_complete_isr_ops = {
    .start = isr_start,
    .stop  = isr_stop,
};

uint8_t sender_isr_init(sender_isr_t *tx, const sender_cfg_t *cfg)
{
    if (!tx || !cfg)
        return 1;

    if (sender_init(&tx->base, cfg) != 0)
        return 1;
    tx->base.complete_ops = &sender_complete_isr_ops;
    return 0;
}
