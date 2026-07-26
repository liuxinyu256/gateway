/**
 * frame_timer_hw.c —— CH579 TMR0-3 硬件定时器适配
 * 换平台: 只替换此文件
 */
#include "frame_timer_hw.h"
#ifdef __CH579__
#include "CH57x_common.h"
#endif

#define HW_MAX  4
#define TICK_US 1000

typedef struct {
    uint16_t (*init)(void);
    void (*start)(void);
    void (*stop)(void);
    void (*restart)(void);
    void (*clear_flag)(void);
} hw_adapter_t;

/* ---- 包装函数 (CH579 TMR 宏 → 函数指针) ---- */
#ifdef __CH579__
static uint16_t t0i(void){TMR0_TimerInit(FREQ_SYS/1000);TMR0_ITCfg(ENABLE,TMR0_3_IT_CYC_END);NVIC_EnableIRQ(TMR0_IRQn);return TICK_US;}
static void t0s(void){TMR0_Enable();} static void t0p(void){TMR0_Disable();}
static void t0r(void){TMR0_Disable();TMR0_Enable();} static void t0c(void){TMR0_ClearITFlag(TMR0_3_IT_CYC_END);}
static uint16_t t1i(void){TMR1_TimerInit(FREQ_SYS/1000);TMR1_ITCfg(ENABLE,TMR0_3_IT_CYC_END);NVIC_EnableIRQ(TMR1_IRQn);return TICK_US;}
static void t1s(void){TMR1_Enable();} static void t1p(void){TMR1_Disable();}
static void t1r(void){TMR1_Disable();TMR1_Enable();} static void t1c(void){TMR1_ClearITFlag(TMR0_3_IT_CYC_END);}
static uint16_t t2i(void){TMR2_TimerInit(FREQ_SYS/1000);TMR2_ITCfg(ENABLE,TMR0_3_IT_CYC_END);NVIC_EnableIRQ(TMR2_IRQn);return TICK_US;}
static void t2s(void){TMR2_Enable();} static void t2p(void){TMR2_Disable();}
static void t2r(void){TMR2_Disable();TMR2_Enable();} static void t2c(void){TMR2_ClearITFlag(TMR0_3_IT_CYC_END);}
static uint16_t t3i(void){TMR3_TimerInit(FREQ_SYS/1000);TMR3_ITCfg(ENABLE,TMR0_3_IT_CYC_END);NVIC_EnableIRQ(TMR3_IRQn);return TICK_US;}
static void t3s(void){TMR3_Enable();} static void t3p(void){TMR3_Disable();}
static void t3r(void){TMR3_Disable();TMR3_Enable();} static void t3c(void){TMR3_ClearITFlag(TMR0_3_IT_CYC_END);}
#else
static uint16_t t0i(void){return TICK_US;} static void t0s(void){} static void t0p(void){} static void t0r(void){} static void t0c(void){}
static uint16_t t1i(void){return TICK_US;} static void t1s(void){} static void t1p(void){} static void t1r(void){} static void t1c(void){}
static uint16_t t2i(void){return TICK_US;} static void t2s(void){} static void t2p(void){} static void t2r(void){} static void t2c(void){}
static uint16_t t3i(void){return TICK_US;} static void t3s(void){} static void t3p(void){} static void t3r(void){} static void t3c(void){}
#endif

static const hw_adapter_t adapter[HW_MAX] = {
    {t0i,t0s,t0p,t0r,t0c},{t1i,t1s,t1p,t1r,t1c},
    {t2i,t2s,t2p,t2r,t2c},{t3i,t3s,t3p,t3r,t3c},
};

typedef struct { frame_timer_t base; uint8_t hw_id; } inst_t;
static inst_t  instances[HW_MAX];
static uint8_t occupied;

static void hw_start(frame_timer_t *t)  {inst_t *s=(inst_t*)t;t->counter=0;adapter[s->hw_id].start();}
static void hw_stop(frame_timer_t *t)   {inst_t *s=(inst_t*)t;adapter[s->hw_id].stop();t->counter=0;}
static void hw_restart(frame_timer_t *t){inst_t *s=(inst_t*)t;t->counter=0;adapter[s->hw_id].restart();}
static void hw_set_cb(frame_timer_t *t, timer_callback cb, void *ctx){t->cb=cb;t->ctx=ctx;}
static const frame_timer_ops_t hw_ops = {hw_start,hw_restart,hw_stop,hw_set_cb};

frame_timer_t* frame_timer_hw_create(uint8_t hw_id) {
    if (hw_id>=HW_MAX||(occupied&(1<<hw_id))) return NULL;
    occupied|=1U<<hw_id;
    inst_t *inst=&instances[hw_id];
    inst->base.ops=&hw_ops;inst->base.cb=NULL;inst->base.ctx=NULL;inst->base.counter=0;
    inst->hw_id=hw_id;
    adapter[hw_id].init();adapter[hw_id].stop();
    return &inst->base;
}

void frame_timer_hw_destroy(frame_timer_t *t){
    if(!t)return;inst_t *s=(inst_t*)t;
    adapter[s->hw_id].stop();t->cb=NULL;t->ctx=NULL;
    occupied&=~(1U<<s->hw_id);
}

void frame_timer_hw_isr(uint8_t hw_id){
    if(!(occupied&(1<<hw_id)))return;
    inst_t *t=&instances[hw_id];
    adapter[hw_id].clear_flag();
    t->base.counter++;
    if(t->base.cb)t->base.cb(t->base.ctx);
}
