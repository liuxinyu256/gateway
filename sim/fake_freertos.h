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

static inline TickType_t xTaskGetTickCount(void) { return 0; }
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

/* Timer */
typedef void (*TimerCallbackFunction_t)(TimerHandle_t);
static inline TimerHandle_t xTimerCreate(const char*n,TickType_t t,int autoReload,void*id,TimerCallbackFunction_t cb){
    (void)n;(void)t;(void)autoReload;(void)id;(void)cb; return (TimerHandle_t)1;
}
static inline BaseType_t xTimerStart(TimerHandle_t t, TickType_t w) { (void)t;(void)w; return pdPASS; }
static inline BaseType_t xTimerChangePeriod(TimerHandle_t t, TickType_t p, TickType_t w) { (void)t;(void)p;(void)w; return pdPASS; }
static inline void *pvTimerGetTimerID(TimerHandle_t t) { (void)t; return NULL; }

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
