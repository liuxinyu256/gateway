/**
 * ac_module.c —— AC 模块 (HVAC RS485)
 *
 * sender / receiver 都由上层指针注入，本模块只管理品牌状态机。
 */
#include "ac_module.h"
#include "ac_test.h"
#include "debug_module.h"
#include <string.h>

/* 品牌注册表: 由清单生成 (单一数据源, 见 ac_module.h)
 * 新建品牌两步: ① 品牌文件定义 cfg → ② 清单加一行 BRAND(枚举名, &cfg) */
const ac_brand_config_t *const brand_table[AC_BRAND_NUM] = {
    [null_0] = NULL, /* 0 位契约占位 */
    AC_BRAND_LIST(AC_BRAND_TABLE)
};

void ac_module_init(ac_module_t *self,
                    const ac_brand_config_t *const *brand_table,
                    uint8_t brand_count)
{
    if (!self) return;

    self->brand_table = brand_table; /* 拿到注册表地址, 只读 */
    self->brand_count = brand_count;
    self->current     = NULL;
    self->locked      = 0;
}

/* module_ops 适配: 让 module_init 能统一调用 ac_module_init */
static uint8_t ac_ops_init(module_t *m, void *cfg)
{
    ac_module_t        *self = (ac_module_t *)m;
    const ac_init_cfg_t *c   = (const ac_init_cfg_t *)cfg;

    if (!self || !c) return 1;

    if (module_base_init(m, c->baudrate) != 0)
        return 1;

    ac_module_init(self, c->brand_table, c->brand_count);
    return 0;
}

static uint8_t *ac_ops_get_rx_buf(module_t *m, uint16_t *size)
{
    ac_module_t *self = (ac_module_t *)m;
    if (!self || !size) return NULL;
    *size = sizeof(self->rx_buf);
    return self->rx_buf;
}

/* ---- AC 模块统一事件日志（品牌里不再打印） ---- */
static const char *ac_event_name(event_type_t type)
{
    switch (type) {
    case EVENT_PERIODIC_SEND: return "periodic";
    case EVENT_RX_FRAME:      return "rx";
    case EVENT_GATEWAY_CMD:   return "cmd";
    case EVENT_NEED_ACK:      return "need_ack";
    case EVENT_SCAN_AC:       return "scan";
    case EVENT_TICK:          return "tick";
    case EVENT_BUS_IDLE:      return "bus_idle";
    default:                  return "?";
    }
}

static void ac_ops_on_event(module_t *m, const event_t *ev)
{
    if (!ev) return;

    /* 通用投帧事件：由 AC send_task 统一调用 sender_send */
    if (ev->type == EVENT_SEND_FRAME) {
        if (m && m->sender && ev->data && ev->len) {
            uint8_t prio = ev->cmd_arg ? SENDER_PRIO_CMD : SENDER_PRIO_NORM;
            sender_send(m->sender, (const uint8_t *)ev->data,
                        ev->len, prio);
        }
        return;
    }

    if (!log_event_enabled())
        return;

    if (ev->type == EVENT_GATEWAY_CMD) {
        log_printf("[ac evt] %s cmd=%u val=%u\r\n",
                     ac_event_name(ev->type),
                     (unsigned)ev->cmd_val, (unsigned)ev->cmd_arg);
    } else {
        log_printf("[ac evt] %s\r\n", ac_event_name(ev->type));
    }
}

/* ---- AC 模块自己注册 IO 回调 ---- */
static ac_module_t *s_ac_self;

static void ac_frame_done(receiver_t *rx, uint16_t len)
{
    (void)rx;
    if (s_ac_self)
        module_rx_frame_done(&s_ac_self->base, len);
}

static void ac_tx_done(sender_t *tx)
{
    (void)tx;
    if (s_ac_self)
        module_tx_done(&s_ac_self->base);
}

static void ac_ops_register_io_callbacks(module_t *m)
{
    if (!m) return;

    s_ac_self = (ac_module_t *)m;

    if (m->receiver)
        receiver_set_callback(m->receiver, ac_frame_done);
    if (m->sender) {
        sender_callbacks_t cbs = { .done = ac_tx_done };
        sender_set_callbacks(m->sender, &cbs);
    }
}

const module_ops_t ac_module_ops = {
    .init                  = ac_ops_init,
    .start                 = NULL,
    .get_rx_buf            = ac_ops_get_rx_buf,
    .register_io_callbacks = ac_ops_register_io_callbacks,
    .on_event              = ac_ops_on_event,
};

/* 激活品牌: 验证已登记 → 绑定事件表 */
void ac_module_register(ac_module_t *self, const ac_brand_config_t *cfg)
{
    if (!self || !cfg || cfg->brand_id >= self->brand_count)
        return;
    if (self->brand_table[cfg->brand_id] != cfg)
        return; /* 未登记进注册表 */

    self->current = cfg;
    self->locked  = 0;
    module_set_handler(&self->base, cfg->evt_table, self);
}

void ac_module_start_scan(ac_module_t *self)
{
    if (!self || !self->base.handler)
        return;

    self->locked = 0;
    if (self->base.handler->on_scan)
        self->base.handler->on_scan(self);
}

void ac_module_lock(ac_module_t *self)
{
    if (self) self->locked = 1;
}

uint8_t ac_module_locked(ac_module_t *self)
{
    return self ? self->locked : 0;
}

const ac_brand_config_t *ac_module_current(ac_module_t *self)
{
    return self->current;
}

void ac_module_set_poll_period(ac_module_t *self, uint16_t period_ms)
{
    (void)self;
    (void)period_ms;
}

/* 把 AC 模块当前完整状态发布到网关 */
void ac_module_publish_state(ac_module_t *self)
{
    if (!self) return;

    module_publish_state(&self->base);
}

/* 更新 AC 模块状态。
 * new_state 必须是由调用方“读当前完整状态 → 只改本协议支持字段”得到的完整状态，
 * 这样才能保证不同协议都只是完整状态的子集。
 */
void ac_module_update_state(ac_module_t *self, const gateway_state_t *new_state)
{
    if (!self || !new_state) return;

    module_update_state(&self->base, new_state);
}
