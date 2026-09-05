/**
 * ac_module.c —— AC 模块 (HVAC RS485)
 *
 * sender / receiver 都由上层指针注入，本模块只管理品牌状态机。
 */
#include "ac_module.h"
#include "debug_module.h"
#include <string.h>

#ifdef __CH579__
/* 临时诊断：RS485 方向切换计数（仅在 CH579 实机编译） */
extern uint32_t ac_phy_rs485_tx_dir_count(void);
extern uint32_t ac_phy_rs485_rx_dir_count(void);
#endif

/* 品牌注册表: 由清单生成 (单一数据源, 见 ac_brand.h)
 * 新建品牌两步: ① 品牌文件定义 cfg → ② 清单加一行 BRAND(枚举名, &cfg) */
const ac_brand_config_t *const brand_table[AC_BRAND_NUM] = {
    [null_0] = NULL, /* 0 位契约占位 */
    AC_BRAND_LIST(AC_BRAND_TABLE)
};

static ac_module_t *s_ac_instance;   /* AC 模块单例 */
static uint8_t      s_ac_auto_tx_enabled = 1; /* 1=允许自动发送 */

static void ac_state_from_gateway(const gateway_state_t *gw, ac_state_t *ac);
static void ac_module_run_state_machine(ac_module_t *self, uint8_t event,
                                        const ac_state_t *state);

/* 初始化 AC 模块：绑定品牌注册表，当前品牌置空 */
void ac_module_init(ac_module_t *self,
                    const ac_brand_config_t *const *brand_table,
                    uint8_t brand_count)
{
    if (!self) return;

    self->brand_table = brand_table; /* 拿到注册表地址, 只读 */
    self->brand_count = brand_count;
    self->current     = NULL;
    self->scan_index  = 0;
    self->locked      = 0;
}

/* module_ops.init：先初始化公共 module，再初始化 AC 品牌表 */
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

/* 提供 AC 模块自己的 RX 缓冲给框架 */
static uint8_t *ac_ops_get_rx_buf(module_t *m, uint16_t *size)
{
    ac_module_t *self = (ac_module_t *)m;
    if (!self || !size) return NULL;
    *size = sizeof(self->rx_buf);
    return self->rx_buf;
}

/* ---- AC 模块统一事件日志（品牌里不再打印） ---- */
/* 事件类型转字符串，用于日志 */
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
    case EVENT_AC_RX:         return "ac_rx";
    default:                  return "?";
    }
}

/* 模块级事件钩子：处理通用投帧事件，其余仅日志 */
static void ac_ops_on_event(module_t *m, const event_t *ev)
{
    if (!ev) return;

    /* 通用投帧事件：由 AC send_task 统一调用 sender_send */
    if (ev->type == EVENT_SEND_FRAME) {
        if (m && m->sender && ev->data && ev->len) {
            uint8_t prio = ev->cmd_arg ? SENDER_PRIO_CMD : SENDER_PRIO_NORM;
            sender_send(m->sender, (const uint8_t *)ev->data, ev->len, prio);
        }
        return;
    }

    /* 接收侧解析完成后投来的品牌事件：由 send_task 跑发送状态机 */
    if (ev->type == EVENT_AC_RX) {
        ac_module_t *self = (ac_module_t *)m;
        ac_state_t ac;

        if (self) {
            ac_state_from_gateway(&self->base.state, &ac);
            ac_module_run_state_machine(self, ev->cmd_val, &ac);
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

/* 网关统一状态 -> AC 协议状态 */
static void ac_state_from_gateway(const gateway_state_t *gw, ac_state_t *ac)
{
    if (!gw || !ac) return;
    ac->power      = gw->power;
    ac->mode       = gw->mode;
    ac->set_temp   = gw->set_temp;
    ac->room_temp  = gw->room_temp;
    ac->fan        = gw->fan;
    ac->swing      = gw->swing;
    ac->error_code = gw->error_code;
}

/* AC 协议状态 -> 网关统一状态 */
static void ac_state_to_gateway(const ac_state_t *ac, gateway_state_t *gw)
{
    if (!ac || !gw) return;
    gw->power      = ac->power;
    gw->mode       = ac->mode;
    gw->set_temp   = ac->set_temp;
    gw->room_temp  = ac->room_temp;
    gw->fan        = ac->fan;
    gw->swing      = ac->swing;
    gw->error_code = ac->error_code;
}

/* 把 AC 协议状态合并进当前完整状态，再走模块统一更新/上报 */
static void ac_module_apply_ac_state(ac_module_t *self, const ac_state_t *ac)
{
    gateway_state_t gw;

    if (!self || !ac)
        return;

    gw = self->base.state;          /* 保留 AC 不管理的字段 */
    ac_state_to_gateway(ac, &gw);   /* 只覆盖 AC 解析到的字段 */
    ac_module_update_state(self, &gw);
}

/* receive_task -> send_task：只投品牌事件号，不传帧数据 */
static uint8_t ac_module_notify_rx_event(ac_module_t *self, uint8_t ev)
{
    if (!self)
        return 1;
    return module_send_event_ex(&self->base, EVENT_AC_RX, ev, 0);
}

/* 模块层统一跑品牌发送状态机：组帧/发送/动态改定时都收口在这里 */
static void ac_module_run_state_machine(ac_module_t *self, uint8_t event,
                                        const ac_state_t *state)
{
    uint8_t  buf[128];
    uint16_t len;
    uint16_t next = 0;

    if (!self || !self->current || !self->current->protocol_ops ||
        !self->current->protocol_ops->state_machine)
        return;

    len = self->current->protocol_ops->state_machine(
              event, state, buf, sizeof(buf), &next);
    if (len)
        ac_module_send_frame(buf, len);
    if (next)
        module_set_poll_period(&self->base, next);
}

/* 使用当前品牌声明的默认轮询周期 */
static void ac_module_apply_brand_period(ac_module_t *self)
{
    if (self && self->current && self->current->protocol_ops &&
        self->current->protocol_ops->poll_period_ms)
        module_set_poll_period(&self->base,
                               self->current->protocol_ops->poll_period_ms);
}

/* 协议 ops：调用品牌组帧函数，拿到帧后由模块层统一发送 */
static void ac_module_tx_send(ac_module_t *self, ac_tx_builder_t builder)
{
    uint8_t buf[128];
    uint16_t len;

    if (!self || !builder)
        return;

    len = builder(buf, sizeof(buf));
    if (len) {
        if (ac_module_send_frame(buf, len) != 0) {
            log_printf("[ac] tx fail len=%u busy=%u sending=%u wait=%u drop=%u\r\n",
                       (unsigned)len,
                       (unsigned)self->base.bus.busy,
                       (unsigned)(self->base.sender ?
                                  self->base.sender->sending : 0),
                       (unsigned)(self->base.sender ?
                                  self->base.sender->wait_tx_complete : 0),
                       (unsigned)(self->base.sender ?
                                  sender_drop_count(self->base.sender) : 0));
        }
    }
}

static void ac_module_scan_next(ac_module_t *self);

/* 发送当前品牌的扫描帧（必须在 send_task 上下文调用） */
static void ac_module_send_scan(ac_module_t *self)
{
    if (self && self->current && self->current->protocol_ops)
        ac_module_tx_send(self, self->current->protocol_ops->on_scan);
}

/* 框架 EVENT_PERIODIC_SEND：锁定后周期查询；未锁定则继续扫描下一个品牌 */
static void ac_module_on_periodic_send(void *ctx)
{
    ac_module_t *self = (ac_module_t *)ctx;

    if (!self)
        return;

    /* 自动周期发送暂停时，保留手动发送能力 */
    if (!s_ac_auto_tx_enabled)
        return;

#ifdef __CH579__
    {
        static uint16_t diag_cnt;
        if ((++diag_cnt % 20) == 0) {
            log_printf("[diag] rs485 dir tx=%u rx=%u\r\n",
                       (unsigned)ac_phy_rs485_tx_dir_count(),
                       (unsigned)ac_phy_rs485_rx_dir_count());
        }
    }
#endif

    if (self->locked) {
        ac_state_t ac;
        ac_state_from_gateway(&self->base.state, &ac);
        ac_module_run_state_machine(self, AC_EV_POLL, &ac);
    } else {
        ac_module_scan_next(self);
    }
}

/* 框架 EVENT_SCAN_AC：在 send_task 里启动/推进扫描 */
static void ac_module_on_scan(void *ctx)
{
    ac_module_t *self = (ac_module_t *)ctx;

    if (!self)
        return;

    if (self->locked || self->current)
        ac_module_send_scan(self);
    else
        ac_module_scan_next(self);
}

/* 扫描状态下切换到下一个候选品牌并发扫描帧；全部试过则回到第一个继续 */
static void ac_module_scan_next(ac_module_t *self)
{
    uint8_t i;

    if (!self || self->locked)
        return;

    for (i = self->scan_index + 1; i < self->brand_count; i++) {
        if (self->brand_table[i]) {
            self->scan_index = i;
            self->current    = self->brand_table[i];
            ac_module_apply_brand_period(self);
            ac_module_send_scan(self);
            return;
        }
    }

    /* 一轮结束仍未锁定：从头再来 */
    self->scan_index = 0;
    self->current    = NULL;
    for (i = 1; i < self->brand_count; i++) {
        if (self->brand_table[i]) {
            self->scan_index = i;
            self->current    = self->brand_table[i];
            ac_module_apply_brand_period(self);
            ac_module_send_scan(self);
            return;
        }
    }
}

/* 框架 EVENT_RX_FRAME（receive_task）：只解析并产生品牌事件 */
static int ac_module_on_rx_frame(void *ctx, uint8_t *data, uint16_t len)
{
    ac_module_t *self = (ac_module_t *)ctx;
    ac_state_t ac;
    uint8_t ev;

    if (!self || !self->current || !self->current->protocol_ops ||
        !self->current->protocol_ops->rx_parse)
        return 1;

    /* 从模块当前状态初始化 AC 状态，协议只覆盖本次解析到的字段 */
    ac_state_from_gateway(&self->base.state, &ac);

    ev = self->current->protocol_ops->rx_parse(data, len, &ac);
    if (ev != AC_EV_NONE) {
        /* 扫描阶段收到本品牌有效应答：锁定当前品牌协议 */
        if (!self->locked) {
            self->locked = 1;
            log_printf("[ac] brand locked id=%u\r\n",
                       (unsigned)self->current->brand_id);
        }

        ac_module_apply_ac_state(self, &ac);

        /* 只投事件号给 send_task，由发送状态机决定下一步 */
        ac_module_notify_rx_event(self, ev);
    }
    return 1;
}

/* 框架 EVENT_GATEWAY_CMD：镜像网关状态，并让品牌构造控制帧发送 */
static void ac_module_on_gateway_cmd(void *ctx, uint8_t cmd, uint8_t val,
                                     const gateway_state_t *state)
{
    ac_module_t   *self = (ac_module_t *)ctx;
    gateway_state_t s;

    if (!self)
        return;

    /* 完整状态同步：AC 模块只镜像网关真相源，并交给发送状态机 */
    if (state) {
        ac_state_t ac;

        self->base.state = *state;
        log_printf("[ac] state sync p=%u m=%u t=%u f=%u\r\n",
                   (unsigned)state->power, (unsigned)state->mode,
                   (unsigned)state->set_temp, (unsigned)state->fan);

        ac_state_from_gateway(state, &ac);
        ac_module_run_state_machine(self, AC_EV_STATE_SYNC, &ac);
        return;
    }

    if (gateway_module_state_get(self->base.module_id, &s) != 0)
        memset(&s, 0, sizeof(s));

    /* 单字段命令：映射到 AC 状态 */
    switch (cmd) {
    case 0: s.power = val; break;
    case 1: s.mode = val; break;
    case 2: s.set_temp = val; break;
    case 3: s.room_temp = val; break;
    case 4: s.fan = val; break;
    case 5: s.swing = val; break;
    default: break;
    }

    ac_module_update_state(self, &s);
}

static const event_handler_t ac_module_evt = {
    .on_periodic_send = ac_module_on_periodic_send,
    .on_rx_frame      = ac_module_on_rx_frame,
    .on_gateway_cmd   = ac_module_on_gateway_cmd,
    .on_scan          = ac_module_on_scan,
};

/* ---- AC 模块自己注册 IO 回调 ---- */
static ac_module_t *s_ac_self;

/* 接收帧完成回调：通知框架产生 EVENT_RX_FRAME */
static void ac_frame_done(receiver_t *rx, uint16_t len)
{
    (void)rx;
    if (s_ac_self)
        module_rx_frame_done(&s_ac_self->base, len);
}

/* 发送完成回调：通知框架进入帧间 gap */
static void ac_tx_done(sender_t *tx)
{
    (void)tx;
    /* sender_poll 软定时器回调运行在硬件定时器 ISR 上下文 */
    if (s_ac_self)
        module_tx_done_from_isr(&s_ac_self->base);
}

/* 注册 AC 物理层接收/发送完成回调 */
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

/* 注册/激活品牌：记录单例实例并绑定模块事件表 */
void ac_module_register(ac_module_t *self, const ac_brand_config_t *cfg)
{
    if (!self || !cfg || cfg->brand_id >= self->brand_count)
        return;
    if (self->brand_table[cfg->brand_id] != cfg)
        return; /* 未登记进注册表 */

    s_ac_instance = self;
    self->current = cfg;
    self->locked  = 0;

    /* 品牌不再自带事件表：统一由 AC 模块层回调处理，再调 protocol_ops */
    module_set_handler(&self->base, &ac_module_evt, self);
}

/* 启动一次扫描/激活流程 */
void ac_module_start_scan(ac_module_t *self)
{
    if (!self || !self->base.handler)
        return;

    /* 自动扫描暂停时保留手动发送能力 */
    if (!s_ac_auto_tx_enabled)
        return;

    self->locked     = 0;
    self->current    = NULL;
    self->scan_index = 0;
    log_printf("[ac] scan start\r\n");
    /* 投到 send_task 再真正发扫描帧，避免 main/调用方栈上 buf 悬空 */
    module_send_event(&self->base, EVENT_SCAN_AC);
}

/* 锁定当前品牌，禁止扫描切换 */
void ac_module_lock(ac_module_t *self)
{
    if (self) self->locked = 1;
}

/* 查询品牌是否锁定 */
uint8_t ac_module_locked(ac_module_t *self)
{
    return self ? self->locked : 0;
}

/* 返回当前激活品牌配置 */
const ac_brand_config_t *ac_module_current(ac_module_t *self)
{
    return self->current;
}

/* 设置 AC 轮询周期：转发给框架的模块定时器 */
void ac_module_set_poll_period(ac_module_t *self, uint16_t period_ms)
{
    if (!self)
        return;
    module_set_poll_period(&self->base, period_ms);
}

/* 把 AC 当前状态上报网关 */
void ac_module_publish_state(ac_module_t *self)
{
    if (!self) return;

    module_publish_state(&self->base);
}

/* 更新 AC 模块状态；new_state 必须是完整状态，有变化才上报网关 */
void ac_module_update_state(ac_module_t *self, const gateway_state_t *new_state)
{
    if (!self || !new_state) return;

    module_update_state(&self->base, new_state);
}

/* 模块级发送：AC 单例，品牌只传帧和长度 */
uint8_t ac_module_send_frame(const uint8_t *frame, uint16_t len)
{
    if (!s_ac_instance)
        return 1;
    return module_send_frame(&s_ac_instance->base, frame, len, SENDER_PRIO_CMD);
}
