#ifndef TIMER_CH579_H
#define TIMER_CH579_H
#include "timer.h"

#ifdef __CH579__

typedef struct {
    uint8_t hw_id;
} ch579_timer_drv_t;

extern const timer_ops_t ch579_timer_ops;
extern ch579_timer_drv_t ch579_timer_drvs[4];

#endif /* __CH579__ */

#endif
