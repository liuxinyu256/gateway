/**
 * module.c —— 事件驱动模块框架
 */

#include "module.h"
#include "receiver_timeout.h"
#include <string.h>

#define MAX_MODULES 5
static module_t *g_modules[MAX_MODULES];

/* 每模块独立的帧回调 */
static void _frame_done(uint8_t id, receiver_t *rx, uint16_t len) {
    module_t *m = g_modules[id];
    if (!m || !m->handler) return;

    if (m->handler->on_rx_isr &&
        m->handler->on_rx_isr(m->handler_ctx, NULL, len)) {
        bus_mark_idle(&m->bus);
        return;
    }

    BaseType_t woken = pdFALSE;
    xTaskNotifyFromISR(m->rx_task, 0, eNoAction, &woken);
    bus_mark_idle(&m->bus);
    portYIELD_FROM_ISR(woken);
}

static void fd_0(receiver_t *r, uint16_t n) { _frame_done(0, r, n); }
static void fd_1(receiver_t *r, uint16_t n) { _frame_done(1, r, n); }
static void fd_2(receiver_t *r, uint16_t n) { _frame_done(2, r, n); }
static void fd_3(receiver_t *r, uint16_t n) { _frame_done(3, r, n); }
static void fd_4(receiver_t *r, uint16_t n) { _frame_done(4, r, n); }

static frame_finish_callback g_frame_cbs[MAX_MODULES] = {
    fd_0, fd_1, fd_2, fd_3, fd_4
};

static const char *g_rx_names[] = {"rx0","rx1","rx2","rx3","rx4"};
static const char *g_tx_names[] = {"tx0","tx1","tx2","tx3","tx4"};

/* rx_task: 只做收 */
static void rx_task_fn(void *pv) {
    module_t *m = (module_t *)pv;
    uint8_t buf[128];

    for (;;) {
        ulTaskNotifyTake(pdTRUE, portMAX_DELAY);
        if (!m->handler || !m->handler->on_rx_frame) continue;

        uint16_t n = ring_count(&m->pkt->ring);
        if (!n) continue;
        if (n > 128) n = 128;

        ring_peek(&m->pkt->ring, buf, n);
        m->handler->on_rx_frame(m->handler_ctx, buf, n);
        ring_skip(&m->pkt->ring, n);
    }
}

/* send_task: 只做发 */
static void send_task_fn(void *pv) {
    module_t *m = (module_t *)pv;
    event_t ev;
    for (;;) {
        xQueueReceive(m->send_queue, &ev, portMAX_DELAY);
        if (!m->handler) continue;
        switch (ev.type) {
        case EVENT_PERIODIC_SEND:
            if (m->handler->on_periodic_send)
                m->handler->on_periodic_send(m->handler_ctx); break;
        case EVENT_CONTROL_CMD:
            if (m->handler->on_control_cmd)
                m->handler->on_control_cmd(m->handler_ctx, ev.cmd_val, ev.cmd_arg); break;
        case EVENT_NEED_ACK:
            if (m->handler->on_need_ack)
                m->handler->on_need_ack(m->handler_ctx); break;
        case EVENT_SCAN_AC:
            if (m->handler->on_scan)
                m->handler->on_scan(m->handler_ctx); break;
        case EVENT_TIMEOUT:
            if (m->handler->on_timeout)
                m->handler->on_timeout(m->handler_ctx); break;
        default: break;
        }
    }
}

static void poll_timer_cb(TimerHandle_t t) {
    module_t *m = (module_t *)pvTimerGetTimerID(t);
    if (!m) return;
    event_t ev = { .type = EVENT_PERIODIC_SEND };
    xQueueSend(m->send_queue, &ev, 0);
}

static void timeout_timer_cb(TimerHandle_t t) {
    module_t *m = (module_t *)pvTimerGetTimerID(t);
    if (!m || !m->handler || !m->handler->on_timeout) return;
    event_t ev = { .type = EVENT_TIMEOUT };
    xQueueSend(m->send_queue, &ev, 0);
}

/* ---- API ---- */
int module_init(module_t *m, uint8_t id, phy_driver_t *phy,
                 uint32_t baudrate, uint16_t ring_size,
                 frame_timer_t *timer, uint16_t timeout_ticks) {
    if (!m || id >= MAX_MODULES || ring_size > RING_HVAC_AC) return -1;

    memset(m, 0, sizeof(*m));
    m->id = id;
    m->phy = phy;
    m->ring_size = ring_size;

    bus_init(&m->bus, baudrate);
    sender_init(&m->sender, phy, &m->bus, m->tx_ring_buf, ring_size);

    m->pkt = receiver_timeout_create(timer, timeout_ticks, g_frame_cbs[id],
                                      m->rx_ring_buf, ring_size);
    if (!m->pkt) return -1;

    g_modules[id] = m;
    return 0;
}

void module_start(module_t *m) {
    if (!m) return;
    m->send_queue = xQueueCreate(8, sizeof(event_t));
    xTaskCreate(rx_task_fn, g_rx_names[m->id], 256, m, 4, &m->rx_task);
    xTaskCreate(send_task_fn, g_tx_names[m->id], 256, m, 3, &m->send_task);
    m->poll_timer = xTimerCreate("poll", pdMS_TO_TICKS(200), pdTRUE, (void *)m, poll_timer_cb);
    xTimerStart(m->poll_timer, 0);
    m->timeout_timer = xTimerCreate("tmo", pdMS_TO_TICKS(50), pdTRUE, (void *)m, timeout_timer_cb);
    xTimerStart(m->timeout_timer, 0);
    m->phy->open(m->phy);
}

int module_send_cmd(module_t *m, uint8_t cmd, uint8_t val) {
    if (!m) return -1;
    event_t ev = { .type = EVENT_CONTROL_CMD, .cmd_val = cmd, .cmd_arg = val };
    return (xQueueSend(m->send_queue, &ev, 0) == pdPASS) ? 0 : -1;
}

void module_set_poll_period(module_t *m, uint16_t period_ms) {
    if (!m || !m->poll_timer) return;
    xTimerChangePeriod(m->poll_timer, pdMS_TO_TICKS(period_ms), 0);
}
