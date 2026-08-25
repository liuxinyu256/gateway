#ifndef FAKE_FREERTOS_H
#define FAKE_FREERTOS_H
#include <stdint.h>
#include <stdlib.h>
#include <string.h>

typedef void*           TaskHandle_t;
typedef void*           QueueHandle_t;
typedef void*           TimerHandle_t;
typedef void*           SemaphoreHandle_t;
typedef long            BaseType_t;
typedef unsigned long   UBaseType_t;
typedef uint32_t        TickType_t;

#define pdTRUE          1
#define pdFALSE         0
#define pdPASS          1
#define portMAX_DELAY   0xFFFFFFFF
#define eNoAction       0
#define eSetValueWithoutOverwrite 0

extern TickType_t fake_now;
static inline TickType_t xTaskGetTickCount(void) { return fake_now; }
#define pdMS_TO_TICKS(ms) ((TickType_t)(ms))
#define portYIELD_FROM_ISR(x) ((void)(x))

/* Task */
static inline BaseType_t xTaskCreate(void(*fn)(void*),const char*n,uint16_t s,void*p,int prio,TaskHandle_t*h){
    (void)fn;(void)n;(void)s;(void)p;(void)prio; if(h)*h=(TaskHandle_t)1; return pdPASS;
}

/* Queue */
static inline QueueHandle_t xQueueCreate(int len, int sz) { (void)len;(void)sz; return (QueueHandle_t)1; }
static inline BaseType_t xQueueSend(QueueHandle_t q, const void *p, TickType_t t) { (void)q;(void)p;(void)t; return pdPASS; }
static inline BaseType_t xQueueReceive(QueueHandle_t q, void *p, TickType_t t) { (void)q;(void)p;(void)t; return pdTRUE; }
static inline BaseType_t xQueueSendFromISR(QueueHandle_t q, const void *p, BaseType_t *w) { (void)q;(void)p; if(w)*w=pdFALSE; return pdPASS; }

/* Timer — 统一定时器接口 (FreeRTOS software timer)
 * 共享数据见 sim/fake_freertos_timer.c, 测试可推进时钟驱动到期 */
typedef void (*TimerCallbackFunction_t)(TimerHandle_t);

typedef struct {
    const char             *name;
    TickType_t              period;
    int                     auto_reload;
    void                   *id;
    TimerCallbackFunction_t cb;
    int                     active;
    TickType_t              deadline;
} fake_timer_t;

#define FAKE_TIMER_MAX 8
extern fake_timer_t fake_timers[FAKE_TIMER_MAX];
extern int          fake_timer_count;

static inline TimerHandle_t xTimerCreate(const char *n, TickType_t p, int ar, void *id, TimerCallbackFunction_t cb)
{
    if (fake_timer_count >= FAKE_TIMER_MAX) return NULL;
    fake_timer_t *ft = &fake_timers[fake_timer_count++];
    ft->name = n; ft->period = p; ft->auto_reload = ar;
    ft->id = id; ft->cb = cb; ft->active = 0; ft->deadline = 0;
    return (TimerHandle_t)ft;
}
static inline BaseType_t xTimerStart(TimerHandle_t t, TickType_t w)
{
    (void)w;
    fake_timer_t *ft = (fake_timer_t *)t;
    ft->active = 1; ft->deadline = fake_now + ft->period;
    return pdPASS;
}
static inline BaseType_t xTimerStartFromISR(TimerHandle_t t, BaseType_t *w)
{
    if (w) *w = pdFALSE;
    return xTimerStart(t, 0);
}
static inline BaseType_t xTimerReset(TimerHandle_t t, TickType_t w)
{
    (void)w;
    fake_timer_t *ft = (fake_timer_t *)t;
    ft->active = 1; ft->deadline = fake_now + ft->period;
    return pdPASS;
}
static inline BaseType_t xTimerStop(TimerHandle_t t, TickType_t w)
{
    (void)w;
    fake_timer_t *ft = (fake_timer_t *)t;
    ft->active = 0;
    return pdPASS;
}
static inline BaseType_t xTimerChangePeriod(TimerHandle_t t, TickType_t p, TickType_t w)
{
    (void)w;
    fake_timer_t *ft = (fake_timer_t *)t;
    ft->period = p;
    return pdPASS;
}
static inline BaseType_t xTimerDelete(TimerHandle_t t, TickType_t w)
{
    (void)w;
    fake_timer_t *ft = (fake_timer_t *)t;
    ft->active = 0; ft->cb = NULL;
    return pdPASS;
}
static inline void *pvTimerGetTimerID(TimerHandle_t t)
{
    return ((fake_timer_t *)t)->id;
}

/* 测试辅助: 推进时钟 / 到期触发 / 复位注册表 */
static inline void fake_timer_advance(TickType_t d) { fake_now += d; }
static inline void fake_timer_fire(TimerHandle_t t)
{
    fake_timer_t *ft = (fake_timer_t *)t;
    if (!ft->active || !ft->cb) return;
    if (fake_now < ft->deadline) return;
    ft->active = 0;
    ft->cb(t);
}
static inline void fake_timer_reset(void)
{
    fake_timer_count = 0; fake_now = 0;
    memset(fake_timers, 0, sizeof(fake_timers));
}

/* Semaphore */
static inline SemaphoreHandle_t xSemaphoreCreateMutex(void) { return (SemaphoreHandle_t)1; }
static inline BaseType_t xSemaphoreTake(SemaphoreHandle_t s, TickType_t t) { (void)s;(void)t; return pdTRUE; }
static inline BaseType_t xSemaphoreGive(SemaphoreHandle_t s) { (void)s; return pdTRUE; }

/* TaskNotify */
static inline BaseType_t xTaskNotifyGive(TaskHandle_t t) { (void)t; return pdPASS; }
static inline uint32_t ulTaskNotifyTake(BaseType_t clear, TickType_t t) { (void)clear;(void)t; return 0; }
static inline BaseType_t xTaskNotifyFromISR(TaskHandle_t t, uint32_t v, int a, BaseType_t *w) {
    (void)t;(void)v;(void)a; if(w)*w=pdFALSE; return pdPASS;
}

/* FreeRTOS.h 兼容 */
#define vTaskStartScheduler() ((void)0)
static inline void* pvPortMalloc(size_t sz) { return malloc(sz); }
static inline void  vPortFree(void *p)      { free(p); }
#endif
