#ifndef TIMER_INSTANCE_H
#define TIMER_INSTANCE_H
#include "timer.h"

/**
 * 4 个定时器全局实例
 * timer0 / timer1 / timer2 / timer3
 *
 * 与 uart_instance 同样式：实机上由具体芯片绑定 ops/drv。
 */

extern timer_t timer0;
extern timer_t timer1;
extern timer_t timer2;
extern timer_t timer3;

timer_t *timer_get(uint8_t id);

#endif
