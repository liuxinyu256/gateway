/**
 * sender_complete_isr.h —— 发送完成策略：中断子类
 *
 * 对应未来 receiver_idle（接收空闲中断子类）。
 */
#ifndef SENDER_COMPLETE_ISR_H
#define SENDER_COMPLETE_ISR_H
#include "sender.h"

typedef struct {
    sender_t base;
    /* 中断策略私有数据（如中断使能标志） */
} sender_isr_t;

extern const sender_ops_t sender_isr_ops;

uint8_t sender_isr_init(sender_isr_t *tx, const sender_cfg_t *cfg);

#endif /* SENDER_COMPLETE_ISR_H */
