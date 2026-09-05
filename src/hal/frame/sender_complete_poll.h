/**
 * sender_complete_poll.h —— 发送完成策略：轮询子类
 *
 * 多个 sender_poll 实例共享一个硬件定时器 tick，
 * 每个实例用 soft_timer_t 独立启停。
 */
#ifndef SENDER_COMPLETE_POLL_H
#define SENDER_COMPLETE_POLL_H
#include "sender.h"
#include "timer_soft.h"

typedef struct {
    sender_t      base;
    soft_timer_t  soft;   /* 共享硬件 tick 上的软定时器实例 */
} sender_poll_t;

extern const sender_ops_t sender_poll_ops;

uint8_t sender_poll_init(sender_poll_t *tx, const sender_cfg_t *cfg);

#endif /* SENDER_COMPLETE_POLL_H */
