#include <stdio.h>
#include <string.h>
#include <assert.h>
#include "receiver.h"
#include "receiver_timeout.h"
#include "frame_timer_sw.h"

typedef struct { frame_timer_t base; uint8_t running; } mock_timer_t;
static void m_start(frame_timer_t *t)  { t->counter=0; ((mock_timer_t*)t)->running=1; }
static void m_restart(frame_timer_t *t){ t->counter=0; }
static void m_stop(frame_timer_t *t)   { t->counter=0; ((mock_timer_t*)t)->running=0; }
static void m_set_cb(frame_timer_t *t, timer_callback cb, void *ctx){ t->cb=cb; t->ctx=ctx; }
static const frame_timer_ops_t m_ops={m_start,m_restart,m_stop,m_set_cb};
static frame_timer_t* mock_init(mock_timer_t *m){ memset(m,0,sizeof(*m)); m->base.ops=&m_ops; return &m->base; }
static void mock_tick(frame_timer_t *t){ mock_timer_t *m=(mock_timer_t*)t; if(!m->running)return; m->base.counter++; if(m->base.cb)m->base.cb(m->base.ctx);}

static uint8_t g_cap[128]; static uint16_t g_len; static int g_cnt;
static void capture(receiver_t *r, uint16_t l){ uint16_t n=l>128?128:l; receiver_read_frame(r,g_cap,128);g_len=n;g_cnt++; }
static void reset(void){ memset(g_cap,0,sizeof(g_cap));g_len=0;g_cnt=0; }

/* 测试使用的 ring buffer */
static uint8_t g_ring_buf[128];
static uint8_t g_ring_buf2[128];

static int run,pass;
#define TEST(n) static void n(void)
#define RUN(n) do{run++;reset();printf("  %s ... ",#n);n();pass++;printf("PASS\n");}while(0)

TEST(t_single_byte){
    mock_timer_t mt;frame_timer_t *t=mock_init(&mt);
    receiver_t *p=receiver_timeout_create(t,3,capture,g_ring_buf,128);
    receiver_put_byte(p,0xAA);assert(t->counter==0);
    mock_tick(t);mock_tick(t);assert(g_cnt==0);
    mock_tick(t);assert(g_cnt==1);assert(g_len==1);assert(g_cap[0]==0xAA);
    receiver_timeout_destroy(p);
}
TEST(t_multi_byte){
    mock_timer_t mt;frame_timer_t *t=mock_init(&mt);
    receiver_t *p=receiver_timeout_create(t,5,capture,g_ring_buf,128);
    uint8_t d[]={1,2,3,4};
    for(int i=0;i<4;i++){receiver_put_byte(p,d[i]);mock_tick(t);mock_tick(t);assert(g_cnt==0);}
    for(int i=0;i<5;i++)mock_tick(t);
    assert(g_cnt==1);assert(g_len==4);assert(memcmp(g_cap,d,4)==0);
    receiver_timeout_destroy(p);
}
TEST(t_restart){
    mock_timer_t mt;frame_timer_t *t=mock_init(&mt);
    receiver_t *p=receiver_timeout_create(t,5,capture,g_ring_buf,128);
    receiver_put_byte(p,0x01);mock_tick(t);mock_tick(t);mock_tick(t);assert(t->counter==3);
    receiver_put_byte(p,0x02);assert(t->counter==0);
    for(int i=0;i<4;i++)mock_tick(t);assert(g_cnt==0);
    mock_tick(t);assert(g_cnt==1);assert(g_len==2);
    receiver_timeout_destroy(p);
}
TEST(t_overflow){
    mock_timer_t mt;frame_timer_t *t=mock_init(&mt);
    receiver_t *p=receiver_timeout_create(t,10,capture,g_ring_buf,128);
    for(int i=0;i<127;i++)assert(receiver_put_byte(p,(uint8_t)i)==0);
    assert(receiver_put_byte(p,0xFF)==1);
    receiver_timeout_destroy(p);
}
TEST(t_reset){
    mock_timer_t mt;frame_timer_t *t=mock_init(&mt);
    receiver_t *p=receiver_timeout_create(t,10,capture,g_ring_buf,128);
    receiver_put_byte(p,1);receiver_put_byte(p,2);receiver_put_byte(p,3);
    assert(ring_count(&p->ring)==3);receiver_reset(p);assert(ring_count(&p->ring)==0);
    receiver_timeout_destroy(p);
}
TEST(t_multi_instance){
    mock_timer_t mt1,mt2;frame_timer_t *t1=mock_init(&mt1),*t2=mock_init(&mt2);
    receiver_t *p1=receiver_timeout_create(t1,3,capture,g_ring_buf,128);
    receiver_t *p2=receiver_timeout_create(t2,3,capture,g_ring_buf2,128);
    receiver_put_byte(p1,0xAA);receiver_put_byte(p2,0xBB);
    for(int i=0;i<3;i++)mock_tick(t1);assert(g_cnt==1);assert(g_cap[0]==0xAA);
    reset();
    for(int i=0;i<3;i++)mock_tick(t2);assert(g_cnt==1);assert(g_cap[0]==0xBB);
    receiver_timeout_destroy(p1);receiver_timeout_destroy(p2);
}
TEST(t_stress){
    mock_timer_t mt;frame_timer_t *t=mock_init(&mt);
    receiver_t *p=receiver_timeout_create(t,10,capture,g_ring_buf,128);
    uint8_t pat[127];for(int i=0;i<127;i++)pat[i]=(uint8_t)i;
    for(int r=0;r<10000;r++){reset();
        for(int i=0;i<127;i++)assert(receiver_put_byte(p,pat[i])==0);
        for(int i=0;i<10;i++)mock_tick(t);
        assert(g_cnt==1);assert(g_len==127);assert(memcmp(g_cap,pat,127)==0);
    }
    receiver_timeout_destroy(p);
}

int main(void){
    printf("\n=== Packetizer Unit Tests ===\n\n");
    RUN(t_single_byte);RUN(t_multi_byte);RUN(t_restart);
    RUN(t_overflow);RUN(t_reset);RUN(t_multi_instance);
    printf("\n--- Stress ---\n");RUN(t_stress);
    printf("\n--- %d/%d passed ---\n",pass,run);
    return pass==run?0:1;
}
