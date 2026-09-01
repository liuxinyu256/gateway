/**
 * sender_complete_isr.c —— 发送完成策略：UART 中断直接完成
 *
 * 适用于有 TX 完成中断的 MCU（如 STM32 TC 中断）。
 * start/stop 用于使能/关闭发送完成中断，具体由平台 UART ISR 调用
 * sender_tx_complete_isr() 或 sender_isr() 完成检测。
 */
#include "sender.h"

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
