#include "timer_instance.h"
#include <stddef.h>

#ifdef __CH579__
#include "timer_ch579.h"

timer_t timer0 = { .ops = &ch579_timer_ops, .drv = &ch579_timer_drvs[0] };
timer_t timer1 = { .ops = &ch579_timer_ops, .drv = &ch579_timer_drvs[1] };
timer_t timer2 = { .ops = &ch579_timer_ops, .drv = &ch579_timer_drvs[2] };
timer_t timer3 = { .ops = &ch579_timer_ops, .drv = &ch579_timer_drvs[3] };
#else
typedef struct {
    uint8_t id;
} timer_drv_t;

static timer_drv_t drv0 = { .id = 0 };
static timer_drv_t drv1 = { .id = 1 };
static timer_drv_t drv2 = { .id = 2 };
static timer_drv_t drv3 = { .id = 3 };

timer_t timer0 = { .drv = &drv0 };
timer_t timer1 = { .drv = &drv1 };
timer_t timer2 = { .drv = &drv2 };
timer_t timer3 = { .drv = &drv3 };
#endif

timer_t *timer_get(uint8_t id)
{
    switch (id) {
    case 0: return &timer0;
    case 1: return &timer1;
    case 2: return &timer2;
    case 3: return &timer3;
    default: return NULL;
    }
}
