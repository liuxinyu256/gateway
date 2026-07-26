#include "receiver_timeout.h"
#include <string.h>
typedef struct { receiver_t base; frame_timer_t *timer; uint16_t timeout_ticks; uint8_t timer_running; } inst_t;
#define MAX_INST 8
static inst_t instances[MAX_INST]; static uint8_t occupied_map;
static void to_init(receiver_t *pkt){ inst_t *s=(inst_t*)pkt; s->timer_running=0; /* ring_init is called externally by factory */ }
static void to_reset(receiver_t *pkt){ inst_t *s=(inst_t*)pkt; if(s->timer&&s->timer->ops)s->timer->ops->stop(s->timer); s->timer_running=0; ring_reset(&pkt->ring); }
static void to_on_byte(receiver_t *pkt){ inst_t *s=(inst_t*)pkt; if(!s->timer_running){ if(s->timer&&s->timer->ops)s->timer->ops->start(s->timer); s->timer_running=1; }else{ if(s->timer&&s->timer->ops)s->timer->ops->restart(s->timer); } }
static const receiver_ops_t timeout_ops={to_init,to_reset,to_on_byte};
static void on_timeout(void *ctx){ inst_t *s=(inst_t*)ctx; if(!s||!s->timer)return; if(s->timer->counter<s->timeout_ticks)return; if(s->timer->ops)s->timer->ops->stop(s->timer); if(s->base.on_frame_finish)s->base.on_frame_finish(&s->base,ring_count(&s->base.ring));s->timer_running=0;}
receiver_t* receiver_timeout_create(frame_timer_t *timer, uint16_t timeout_ticks, frame_finish_callback cb, uint8_t *ring_buf, uint16_t ring_size) {
    if(!timer||!ring_buf||!ring_size)return NULL;
    for(uint8_t i=0;i<MAX_INST;i++){ if(!(occupied_map&(1<<i))){ occupied_map|=(uint8_t)(1U<<i);
    inst_t *s=&instances[i]; memset(s,0,sizeof(*s)); s->base.ops=&timeout_ops; s->timer=timer; s->timeout_ticks=timeout_ticks; s->base.on_frame_finish=cb; 
    ring_init(&s->base.ring, ring_buf, ring_size);
    if(timer->ops&&timer->ops->set_callback)timer->ops->set_callback(timer,on_timeout,&s->base);
    return &s->base; } }
    return NULL;
}
void receiver_timeout_destroy(receiver_t *pkt){ if(!pkt)return; for(uint8_t i=0;i<MAX_INST;i++){ if(&instances[i].base==pkt){ inst_t *s=&instances[i]; if(s->timer&&s->timer->ops&&s->timer->ops->set_callback)s->timer->ops->set_callback(s->timer,NULL,NULL); s->timer=NULL; s->base.on_frame_finish=NULL; occupied_map&=~(1U<<i); return; } } }
