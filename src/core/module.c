/**
 * module.c —— 事件驱动模块框架
 *
 * 每个 module_t 是一条独立总线/通道：
 *   - bus 负责总线空闲/占用
 *   - sender 负责帧级发送（上层注入）
 *   - receiver 负责帧级接收（上层注入）
 *   - receive_task(P4) 收帧解析
 *   - send_task(P3) 发帧/控制/超时
 *
 * 事件表 (event_handler_t) 由子类或品牌注册，所有回调必须立即返回。
 */
#include "module.h"
#include <string.h>

static module_t *g_modules[MODULE_MAX];

static module_t *module_from_receiver(const receiver_t *receiver)
{
    if (!receiver) return NULL;
    for (uint8_t i = 0; i < MODULE_MAX; i++) {
        if (g_modules[i] && g_modules[i]->receiver == receiver)
            return g_modules[i];
    }
    return NULL;
}

/* ---- 入队 (FreeRTOS 队列 / PC 模拟队列统一入口) ---- */
static uint8_t module_enqueue_send_event(module_t *m, const event_t *ev)
{
    if (!m || !ev) return 1;

#ifdef FAKE_FREERTOS
    if (m->send_q_count >= MODULE_EVENT_QUEUE_LEN) {
        m->send_queue_drop_cnt++;
        return 1;
    }
    m->send_q_data[m->send_q_tail] = *ev;
    m->send_q_tail = (uint8_t)((m->send_q_tail + 1) % MODULE_EVENT_QUEUE_LEN);
    m->send_q_count++;
    return 0;
#else
    if (xQueueSend(m->send_queue, ev, 0) != pdPASS) {
        m->send_queue_drop_cnt++;
        return 1;
    }
    return 0;
#endif
}

#ifdef FAKE_FREERTOS
static uint8_t module_enqueue_receive_event(module_t *m, const event_t *ev)
{
    if (!m || !ev) return 1;

    if (m->receive_q_count >= MODULE_EVENT_QUEUE_LEN) {
        m->receive_queue_drop_cnt++;
        return 1;
    }
    m->receive_q_data[m->receive_q_tail] = *ev;
    m->receive_q_tail = (uint8_t)((m->receive_q_tail + 1) % MODULE_EVENT_QUEUE_LEN);
    m->receive_q_count++;
    return 0;
}
#endif

/* ---- 事件分发 (任务 / PC 轮询共用) ---- */
static void module_handle_rx(module_t *m);

static void module_handle_event(module_t *m, const event_t *ev)
{
    if (!m || !ev) return;

    switch (ev->type) {
    case EVENT_PERIODIC_SEND:
        if (m->handler && m->handler->on_periodic_send)
            m->handler->on_periodic_send(m->handler_ctx);
        break;
    case EVENT_RX_FRAME:
        module_handle_rx(m);
        break;
    case EVENT_CONTROL_CMD:
        if (m->handler && m->handler->on_control_cmd)
            m->handler->on_control_cmd(m->handler_ctx, ev->cmd_val, ev->cmd_arg);
        break;
    case EVENT_NEED_ACK:
        if (m->handler && m->handler->on_need_ack)
            m->handler->on_need_ack(m->handler_ctx);
        break;
    case EVENT_SCAN_AC:
        if (m->handler && m->handler->on_scan)
            m->handler->on_scan(m->handler_ctx);
        break;
    case EVENT_TIMEOUT:
        if (m->handler && m->handler->on_timeout)
            m->handler->on_timeout(m->handler_ctx);
        break;
    case EVENT_BUS_IDLE:
        if (m->sender)
            sender_pump(m->sender);
        break;
    default:
        break;
    }
}

/* ---- RX 处理 ---- */
static void module_handle_rx(module_t *m)
{
    if (!m || !m->receiver) return;

    uint8_t buf[128];
    uint16_t n = receiver_read_frame(m->receiver, buf, sizeof(buf));
    if (n && m->handler && m->handler->on_rx_frame)
        m->handler->on_rx_frame(m->handler_ctx, buf, n);
}

/* ---- 帧完成回调 ---- */
#ifdef FAKE_FREERTOS
static void frame_done_cb(receiver_t *receiver, uint16_t len)
{
    module_t *m = module_from_receiver(receiver);
    if (!m) return;

    event_t ev = {
        .type = EVENT_RX_FRAME,
        .len  = len,
    };
    module_enqueue_receive_event(m, &ev);
}
#else
static void frame_done_cb(receiver_t *receiver, uint16_t len)
{
    module_t *m = module_from_receiver(receiver);
    if (!m || !m->receive_queue) return;

    event_t ev = {
        .type = EVENT_RX_FRAME,
        .len  = len,
    };

    BaseType_t woken = pdFALSE;
    xQueueSendFromISR(m->receive_queue, &ev, &woken);
    portYIELD_FROM_ISR(woken);
}
#endif

/* ---- tx_done: 当前帧发完，启动 gap 定时器 ---- */
#ifdef FAKE_FREERTOS
static void tx_done_cb(void *ctx)
{
    module_t *m = (module_t *)ctx;
    if (m && m->gap_timer)
        xTimerStart(m->gap_timer, 0);
}
#else
static void tx_done_cb(void *ctx)
{
    module_t *m = (module_t *)ctx;
    BaseType_t woken = pdFALSE;

    if (m && m->gap_timer)
        xTimerStartFromISR(m->gap_timer, &woken);

    portYIELD_FROM_ISR(woken);
}
#endif

/* ---- gap 到期: 投递 EVENT_BUS_IDLE ---- */
static void gap_timer_cb(TimerHandle_t t)
{
    module_t *m = (module_t *)pvTimerGetTimerID(t);
    if (!m) return;

    event_t ev = { .type = EVENT_BUS_IDLE };
    module_enqueue_send_event(m, &ev);
}

/* ---- RS485: THR 已空，开始等 TX_COMPLETE ---- */
#ifdef FAKE_FREERTOS
static void wait_tx_complete_cb(void *ctx)
{
    module_t *m = (module_t *)ctx;
    if (m && m->tx_poll_timer)
        xTimerStart(m->tx_poll_timer, 0);
}
#else
static void wait_tx_complete_cb(void *ctx)
{
    module_t *m = (module_t *)ctx;
    BaseType_t woken = pdFALSE;

    if (m && m->tx_poll_timer)
        xTimerStartFromISR(m->tx_poll_timer, &woken);

    portYIELD_FROM_ISR(woken);
}
#endif

/* ---- 1ms 后轮询 TX_COMPLETE ---- */
static void tx_poll_timer_cb(TimerHandle_t t)
{
    module_t *m = (module_t *)pvTimerGetTimerID(t);
    if (!m || !m->sender)
        return;

    /* 返回 1 = 还在等 TX_COMPLETE，下一毫秒继续看 */
    if (sender_poll_tx_complete(m->sender))
        xTimerStart(m->tx_poll_timer, 0);
}

/* ---- FreeRTOS 任务 ---- */
static void receive_task_fn(void *pv)
{
    module_t *m = (module_t *)pv;
    event_t ev;

    for (;;) {
        if (xQueueReceive(m->receive_queue, &ev, portMAX_DELAY) == pdPASS)
            module_handle_event(m, &ev);
    }
}

static void send_task_fn(void *pv)
{
    module_t *m = (module_t *)pv;
    event_t ev;

    for (;;) {
        if (xQueueReceive(m->send_queue, &ev, portMAX_DELAY) == pdPASS)
            module_handle_event(m, &ev);
    }
}

/* ---- 软件定时器回调 ---- */
static void poll_timer_cb(TimerHandle_t t)
{
    module_t *m = (module_t *)pvTimerGetTimerID(t);
    if (!m) return;

    event_t ev = { .type = EVENT_PERIODIC_SEND };
    module_enqueue_send_event(m, &ev);
}

static void timeout_timer_cb(TimerHandle_t t)
{
    module_t *m = (module_t *)pvTimerGetTimerID(t);
    if (!m) return;

    event_t ev = { .type = EVENT_TIMEOUT };
    module_enqueue_send_event(m, &ev);
}

/* ---- API ---- */

/* 公共初始化: 由每个模块自己的 init 函数内部调用 */
uint8_t module_base_init(module_t *m, uint32_t baudrate)
{
    if (!m) return 1;

    bus_init(&m->bus, baudrate);

#ifndef FAKE_FREERTOS
    m->send_queue = xQueueCreate(MODULE_EVENT_QUEUE_LEN, sizeof(event_t));
    if (!m->send_queue) return 1;

    m->receive_queue = xQueueCreate(MODULE_EVENT_QUEUE_LEN, sizeof(event_t));
    if (!m->receive_queue) return 1;
#endif

    for (uint8_t i = 0; i < MODULE_MAX; i++) {
        if (!g_modules[i]) {
            g_modules[i]  = m;
            m->module_id  = i;
            return 0;
        }
    }
    return 1;
}

/* 统一入口: 只负责调用本模块自己的 init */
uint8_t module_init(module_t *m, void *cfg)
{
    if (!m || !m->ops || !m->ops->init)
        return 1;
    return m->ops->init(m, cfg);
}

void module_set_handler(module_t *m, const event_handler_t *handler, void *ctx)
{
    if (!m) return;
    m->handler     = handler;
    m->handler_ctx = ctx;
}

/* 模块状态变化后上报网关（统一走状态事件队列） */
void module_publish_state(module_t *m)
{
    if (!m) return;
    gateway_module_state_update(m->module_id, &m->state);
}

/* 更新模块完整状态：
 * new_state 必须是由调用方“读当前完整状态 → 只改本协议支持字段”得到的完整状态
 */
void module_update_state(module_t *m, const gateway_state_t *new_state)
{
    if (!m || !new_state) return;

    if (memcmp(&m->state, new_state, sizeof(m->state)) == 0)
        return;

    m->state = *new_state;
    module_publish_state(m);
}

void module_start(module_t *m)
{
    if (!m) return;

    xTaskCreate(receive_task_fn, "rx", 128, m, 4, &m->receive_task);
    xTaskCreate(send_task_fn, "tx", 128, m, 3, &m->send_task);

    if (m->receiver)
        receiver_set_callback(m->receiver, frame_done_cb);

    if (m->sender) {
        sender_callbacks_t cbs = {
            .done             = tx_done_cb,
            .done_ctx         = m,
            .wait_tx_complete = wait_tx_complete_cb,
            .wait_ctx         = m,
        };
        sender_set_callbacks(m->sender, &cbs);
    }

    m->poll_timer = xTimerCreate("poll", pdMS_TO_TICKS(200), pdTRUE,
                                 (void *)m, poll_timer_cb);
    if (m->poll_timer)
        xTimerStart(m->poll_timer, 0);

    m->timeout_timer = xTimerCreate("tmo", pdMS_TO_TICKS(50), pdTRUE,
                                    (void *)m, timeout_timer_cb);
    if (m->timeout_timer)
        xTimerStart(m->timeout_timer, 0);

    m->gap_timer = xTimerCreate("gap", pdMS_TO_TICKS(m->bus.gap_ms),
                                pdFALSE, (void *)m, gap_timer_cb);

    m->tx_poll_timer = xTimerCreate("txpoll", pdMS_TO_TICKS(1),
                                    pdFALSE, (void *)m, tx_poll_timer_cb);

    if (m->ops && m->ops->start)
        m->ops->start(m);
}

uint8_t module_send_cmd(module_t *m, uint8_t cmd, uint8_t val)
{
    if (!m) return 1;

    event_t ev = {
        .type    = EVENT_CONTROL_CMD,
        .cmd_val = cmd,
        .cmd_arg = val,
    };

    return module_enqueue_send_event(m, &ev);
}

void module_set_poll_period(module_t *m, uint16_t period_ms)
{
    if (!m || !m->poll_timer) return;
    xTimerChangePeriod(m->poll_timer, pdMS_TO_TICKS(period_ms), 0);
}

#ifdef FAKE_FREERTOS
uint8_t module_poll(module_t *m)
{
    if (!m) return 0;

    uint8_t processed = 0;

    if (m->receive_q_count > 0) {
        event_t ev = m->receive_q_data[m->receive_q_head];
        m->receive_q_head = (uint8_t)((m->receive_q_head + 1) % MODULE_EVENT_QUEUE_LEN);
        m->receive_q_count--;
        module_handle_event(m, &ev);
        processed = 1;
    }

    if (m->send_q_count > 0) {
        event_t ev = m->send_q_data[m->send_q_head];
        m->send_q_head = (uint8_t)((m->send_q_head + 1) % MODULE_EVENT_QUEUE_LEN);
        m->send_q_count--;
        module_handle_event(m, &ev);
        processed = 1;
    }

    gateway_poll_state_events();

    return processed;
}
#endif
