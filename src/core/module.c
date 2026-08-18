/**
 * module.c —— 事件驱动模块框架
 *
 * 每个 module_t 是一条独立总线/通道：
 *   - bus 负责总线空闲/占用
 *   - sender 负责 ISR 发送管线
 *   - rx_task(P4) 负责收帧解析
 *   - send_task(P3) 负责发帧/控制/超时
 *
 * 事件表 (event_handler_t) 由子类或品牌注册，所有回调必须立即返回。
 */
#include "module.h"
#include <string.h>

static module_t *g_modules[MODULE_MAX];

static module_t *module_from_rx(const receiver_t *rx)
{
    if (!rx) return NULL;
    for (uint8_t i = 0; i < MODULE_MAX; i++) {
        if (g_modules[i] && g_modules[i]->rx == rx)
            return g_modules[i];
    }
    return NULL;
}

/* ---- 入队 (FreeRTOS 队列 / PC 模拟队列统一入口) ---- */
static int module_enqueue_event(module_t *m, const event_t *ev)
{
    if (!m || !ev) return -1;

#ifdef FAKE_FREERTOS
    if (m->send_q_count >= MODULE_SEND_QUEUE_LEN) return -1;
    m->send_q_data[m->send_q_tail] = *ev;
    m->send_q_tail = (m->send_q_tail + 1) % MODULE_SEND_QUEUE_LEN;
    m->send_q_count++;
    return 0;
#else
    return (xQueueSend(m->send_queue, ev, 0) == pdPASS) ? 0 : -1;
#endif
}

/* ---- 事件分发 (send_task / PC 轮询共用) ---- */
static void module_handle_event(module_t *m, const event_t *ev)
{
    if (!m || !m->handler || !ev) return;

    switch (ev->type) {
    case EVENT_PERIODIC_SEND:
        if (m->handler->on_periodic_send)
            m->handler->on_periodic_send(m->handler_ctx);
        break;
    case EVENT_CONTROL_CMD:
        if (m->handler->on_control_cmd)
            m->handler->on_control_cmd(m->handler_ctx, ev->cmd_val, ev->cmd_arg);
        break;
    case EVENT_NEED_ACK:
        if (m->handler->on_need_ack)
            m->handler->on_need_ack(m->handler_ctx);
        break;
    case EVENT_SCAN_AC:
        if (m->handler->on_scan)
            m->handler->on_scan(m->handler_ctx);
        break;
    case EVENT_TIMEOUT:
        if (m->handler->on_timeout)
            m->handler->on_timeout(m->handler_ctx);
        break;
    default:
        break;
    }
}

/* ---- RX 处理 (rx_task / PC 轮询共用) ---- */
static void module_handle_rx(module_t *m)
{
    if (!m || !m->rx) return;

    uint8_t buf[128];
    uint16_t n = receiver_read_frame(m->rx, buf, sizeof(buf));
    if (n && m->handler && m->handler->on_rx_frame)
        m->handler->on_rx_frame(m->handler_ctx, buf, n);
}

/* ---- 帧完成回调 ---- */
#ifdef FAKE_FREERTOS
static void frame_done_cb(receiver_t *rx, uint16_t len)
{
    (void)len;
    module_t *m = module_from_rx(rx);
    if (m) m->rx_pending = 1;
}
#else
static void frame_done_cb(receiver_t *rx, uint16_t len)
{
    (void)len;
    module_t *m = module_from_rx(rx);
    if (!m || !m->rx_task) return;

    BaseType_t woken = pdFALSE;
    xTaskNotifyFromISR(m->rx_task, 0, eNoAction, &woken);
    portYIELD_FROM_ISR(woken);
}
#endif

/* ---- FreeRTOS 任务 ---- */
static void rx_task_fn(void *pv)
{
    module_t *m = (module_t *)pv;
    for (;;) {
        ulTaskNotifyTake(pdTRUE, portMAX_DELAY);
        module_handle_rx(m);
    }
}

static void send_task_fn(void *pv)
{
    module_t *m = (module_t *)pv;
    event_t ev;
    for (;;) {
        if (xQueueReceive(m->send_queue, &ev, portMAX_DELAY) != pdPASS)
            continue;
        module_handle_event(m, &ev);
    }
}

/* ---- 软件定时器回调 ---- */
static void poll_timer_cb(TimerHandle_t t)
{
    module_t *m = (module_t *)pvTimerGetTimerID(t);
    if (!m) return;

    event_t ev = { .type = EVENT_PERIODIC_SEND };
    module_enqueue_event(m, &ev);
}

static void timeout_timer_cb(TimerHandle_t t)
{
    module_t *m = (module_t *)pvTimerGetTimerID(t);
    if (!m) return;

    event_t ev = { .type = EVENT_TIMEOUT };
    module_enqueue_event(m, &ev);
}

/* ---- API ---- */

/* 公共初始化: 由每个模块自己的 init 函数内部调用 */
int module_base_init(module_t *m, uint32_t baudrate)
{
    if (!m) return -1;

    bus_init(&m->bus, baudrate);

#ifndef FAKE_FREERTOS
    if (!m->send_queue) {
        m->send_queue = xQueueCreate(MODULE_SEND_QUEUE_LEN, sizeof(event_t));
        if (!m->send_queue) return -1;
    }
#endif

    for (uint8_t i = 0; i < MODULE_MAX; i++) {
        if (!g_modules[i]) {
            g_modules[i] = m;
            return 0;
        }
    }
    return -1;
}

/* 统一入口: 只负责调用本模块自己的 init */
int module_init(module_t *m, void *cfg)
{
    if (!m || !m->ops || !m->ops->init)
        return -1;
    return m->ops->init(m, cfg);
}

void module_set_handler(module_t *m, const event_handler_t *handler, void *ctx)
{
    if (!m) return;
    m->handler     = handler;
    m->handler_ctx = ctx;
}

void module_start(module_t *m)
{
    if (!m) return;

    xTaskCreate(rx_task_fn, "rx", 256, m, 4, &m->rx_task);
    xTaskCreate(send_task_fn, "tx", 256, m, 3, &m->send_task);

    if (m->rx)
        receiver_set_callback(m->rx, frame_done_cb);

    m->poll_timer = xTimerCreate("poll", pdMS_TO_TICKS(200), pdTRUE,
                                 (void *)m, poll_timer_cb);
    if (m->poll_timer)
        xTimerStart(m->poll_timer, 0);

    m->timeout_timer = xTimerCreate("tmo", pdMS_TO_TICKS(50), pdTRUE,
                                    (void *)m, timeout_timer_cb);
    if (m->timeout_timer)
        xTimerStart(m->timeout_timer, 0);

    if (m->ops && m->ops->start)
        m->ops->start(m);
}

int module_send_cmd(module_t *m, uint8_t cmd, uint8_t val)
{
    if (!m) return -1;

    event_t ev = {
        .type    = EVENT_CONTROL_CMD,
        .cmd_val = cmd,
        .cmd_arg = val,
    };

    return module_enqueue_event(m, &ev);
}

void module_set_poll_period(module_t *m, uint16_t period_ms)
{
    if (!m || !m->poll_timer) return;
    xTimerChangePeriod(m->poll_timer, pdMS_TO_TICKS(period_ms), 0);
}

#ifdef FAKE_FREERTOS
int module_poll(module_t *m)
{
    if (!m) return -1;

    int processed = 0;

    if (m->rx_pending) {
        m->rx_pending = 0;
        module_handle_rx(m);
        processed = 1;
    }

    if (m->send_q_count > 0) {
        event_t ev = m->send_q_data[m->send_q_head];
        m->send_q_head = (m->send_q_head + 1) % MODULE_SEND_QUEUE_LEN;
        m->send_q_count--;
        module_handle_event(m, &ev);
        processed = 1;
    }

    return processed;
}
#endif
