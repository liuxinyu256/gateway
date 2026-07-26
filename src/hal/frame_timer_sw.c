#include "frame_timer_sw.h"
#include <stddef.h>
#ifdef _WIN32
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
static uint64_t get_us(void){static LARGE_INTEGER f={0};static int ok=0;if(!ok){QueryPerformanceFrequency(&f);ok=1;}LARGE_INTEGER c;QueryPerformanceCounter(&c);return (uint64_t)((c.QuadPart*1000000ULL)/f.QuadPart);}
void frame_timer_sw_sleep_us(uint64_t us){if(us>2000){Sleep((DWORD)(us/1000));}else{uint64_t d=get_us()+us;while(get_us()<d);}}
#else
#include <time.h>
#include <unistd.h>
static uint64_t get_us(void){struct timespec ts;clock_gettime(CLOCK_MONOTONIC,&ts);return (uint64_t)ts.tv_sec*1000000ULL+(uint64_t)ts.tv_nsec/1000ULL;}
void frame_timer_sw_sleep_us(uint64_t us){if(us>2000){usleep((useconds_t)us);}else{uint64_t d=get_us()+us;while(get_us()<d);}}
#endif
uint64_t frame_timer_sw_now_us(void){return get_us();}
#define FT_SW_MAX 4
typedef struct{frame_timer_t base;uint8_t sw_id;uint32_t tick_period_us;int running;uint64_t next_tick_us;}inst_t;
static inst_t insts[FT_SW_MAX];static uint8_t occ;
static void s_start(frame_timer_t *t){inst_t *s=(inst_t*)t;t->counter=0;s->running=1;s->next_tick_us=get_us()+s->tick_period_us;}
static void s_restart(frame_timer_t *t){inst_t *s=(inst_t*)t;t->counter=0;s->next_tick_us=get_us()+s->tick_period_us;}
static void s_stop(frame_timer_t *t){inst_t *s=(inst_t*)t;t->counter=0;s->running=0;}
static void s_set_cb(frame_timer_t *t,timer_callback cb,void *ctx){t->cb=cb;t->ctx=ctx;}
static const frame_timer_ops_t s_ops={s_start,s_restart,s_stop,s_set_cb};
frame_timer_t* frame_timer_sw_create(uint32_t tp){
    for(uint8_t i=0;i<FT_SW_MAX;i++){if(!(occ&(1U<<i))){occ|=1U<<i;inst_t *s=&insts[i];
    s->base.ops=&s_ops;s->base.cb=NULL;s->base.ctx=NULL;s->base.counter=0;
    s->sw_id=i;s->tick_period_us=tp?tp:1000;s->running=0;s->next_tick_us=0;return &s->base;}}
    return NULL;
}
void frame_timer_sw_destroy(frame_timer_t *t){if(!t)return;inst_t *s=(inst_t*)t;uint8_t id=s->sw_id;if(id>=FT_SW_MAX)return;s->running=0;t->cb=NULL;t->ctx=NULL;occ&=~(1U<<id);}
void frame_timer_sw_poll(frame_timer_t *t){
    if(!t)return;inst_t *s=(inst_t*)t;if(!s->running)return;
    uint64_t now=get_us();int tc=0;
    while(now>=s->next_tick_us&&s->running){t->counter++;s->next_tick_us+=s->tick_period_us;if(t->cb)t->cb(t->ctx);if(!s->running)break;if(++tc>1000){s->next_tick_us=now+s->tick_period_us;break;}}
}
void frame_timer_sw_poll_all(void){for(uint8_t i=0;i<FT_SW_MAX;i++){if(occ&(1U<<i))frame_timer_sw_poll(&insts[i].base);}}
