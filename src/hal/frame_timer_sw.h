#ifndef FRAME_TIMER_SW_H
#define FRAME_TIMER_SW_H
#include "frame_timer.h"
frame_timer_t* frame_timer_sw_create(uint32_t tick_period_us);
void frame_timer_sw_destroy(frame_timer_t *t);
void frame_timer_sw_poll(frame_timer_t *t);
void frame_timer_sw_poll_all(void);
uint64_t frame_timer_sw_now_us(void);
void frame_timer_sw_sleep_us(uint64_t us);
#endif
