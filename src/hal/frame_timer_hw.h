/**
 * frame_timer_hw.h —— 硬件定时器工厂 (平台相关)
 *
 * hw_id: 0=TMR0, 1=TMR1, 2=TMR2, 3=TMR3
 * 换平台只需替换 frame_timer_hw.c 中的 hw_adapter 表
 */

#ifndef FRAME_TIMER_HW_H
#define FRAME_TIMER_HW_H

#include "frame_timer.h"

frame_timer_t* frame_timer_hw_create(uint8_t hw_id);
void           frame_timer_hw_destroy(frame_timer_t *t);
void           frame_timer_hw_isr(uint8_t hw_id);

#endif
