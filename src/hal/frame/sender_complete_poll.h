/**
 * sender_complete_poll.h —— 发送完成策略：轮询子类
 *
 * 对应 receiver_timeout_t（接收超时子类），
 * 一个策略一个类。
 */
#ifndef SENDER_COMPLETE_POLL_H
#define SENDER_COMPLETE_POLL_H
#include "sender.h"

typedef struct {
    sender_t base;
    void    *timer;   /* TimerHandle_t：1ms 轮询定时器 */
} sender_poll_t;

uint8_t sender_poll_init(sender_poll_t *tx, const sender_cfg_t *cfg);

#endif /* SENDER_COMPLETE_POLL_H */
