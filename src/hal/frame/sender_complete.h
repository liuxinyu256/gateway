/**
 * sender_complete.h —— 发送完成策略接口
 *
 * 与 receiver 的 framing 策略对称：
 *   - sender_complete_poll_ops：无 TX 完成中断，软件定时器轮询
 *   - sender_complete_isr_ops：有 TX 完成中断，ISR 直接完成
 */
#ifndef SENDER_COMPLETE_H
#define SENDER_COMPLETE_H

typedef struct sender sender_t;

typedef struct sender_complete_ops {
    void (*start)(sender_t *tx);  /* THR 空后开始等待 TX_COMPLETE */
    void (*stop)(sender_t *tx);   /* 完成/取消时停止等待 */
} sender_complete_ops_t;

extern const sender_complete_ops_t sender_complete_poll_ops;
extern const sender_complete_ops_t sender_complete_isr_ops;

#endif /* SENDER_COMPLETE_H */
