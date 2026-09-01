/**
 * receiver_timeout.h —— 超时策略接收器 (receiver 子类)
 * 基类 receiver 只提供基础接口; 本类叠加帧间隙超时:
 * 定时器由上层创建并注入 (真实硬件用 timer_hw_create 独占一个定时器),
 * 每字节重置计数值, 计数值到阈值 → 帧完成回调
 */
#ifndef RECEIVER_TIMEOUT_H
#define RECEIVER_TIMEOUT_H
#include "receiver.h"
#include "timer.h"

typedef struct {
    receiver_t base;
    timer_t   *timer;         /* 注入的定时器 (专用绑定, 程序员指定编号) */
    uint16_t   timeout_ticks; /* 超时阈值 (定时器周期数) */
    volatile uint8_t timer_running;
} receiver_timeout_t;

void receiver_timeout_init(receiver_timeout_t *self,
                           timer_t *timer,
                           uint16_t timeout_ticks,
                           frame_finish_callback cb,
                           uint8_t *ring_buf, uint16_t ring_size);
#endif
