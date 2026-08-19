/**
 * ac_module.c —— AC 模块 (HVAC RS485)
 * 接收路径: 基类接收器在 init 时初始化 (实例 ac 模块私有)
 */
#include "ac_module.h"

/* 品牌注册表: 由清单生成 (单一数据源, 见 ac_module.h)
 * 新建品牌两步: ① 品牌文件定义 cfg → ② 清单加一行 BRAND(枚举名, &cfg) */
const ac_brand_config_t *const brand_table[AC_BRAND_NUM] = {
    [null_0] = NULL, /* 0 位契约占位 */
    AC_BRAND_LIST(AC_BRAND_TABLE)
};

void ac_module_init(ac_module_t *self, module_t *m,
                    void (*write_byte)(uint8_t byte),
                    const ac_brand_config_t *const *brand_table,
                    uint8_t brand_count)
{
    if (!self || !m) return;

    self->mod = m;

    /* 发送管线: 缓冲区由子类提供 (各模块大小可不同) */
    sender_init(&m->sender, write_byte, &m->bus,
                self->tx_ring_buf, sizeof(self->tx_ring_buf));

    /* 接收器不在这里创建: 由上层初始化后注入 m->rx */

    self->brand_table = brand_table; /* 拿到注册表地址, 只读 */
    self->brand_count = brand_count;
    self->current     = NULL;
    self->locked      = 0;
}

/* module_ops 适配: 让 module_init 能统一调用 ac_module_init */
static int ac_ops_init(module_t *m, void *cfg)
{
    ac_module_t        *self = (ac_module_t *)m;
    const ac_init_cfg_t *c   = (const ac_init_cfg_t *)cfg;

    if (!self || !c) return -1;

    if (module_base_init(m, c->baudrate) != 0)
        return -1;

    ac_module_init(self, m, c->write_byte,
                   c->brand_table, c->brand_count);

    if (c->uart) {
        /* 挂 UART+decoder: 要求上层已把 m->rx 注入好 */
        if (module_attach_uart(m, c->uart, &c->uart_cfg) != 0)
            return -1;
    }

    return 0;
}

const module_ops_t ac_module_ops = {
    .init  = ac_ops_init,
    .start = NULL,
};

/* 激活品牌: 验证已登记 → 超时配置生效 → 绑定事件表 */
void ac_module_register(ac_module_t *self, const ac_brand_config_t *cfg)
{
    if (!self || !cfg || !self->mod || cfg->brand_id >= self->brand_count)
        return;
    if (self->brand_table[cfg->brand_id] != cfg)
        return; /* 未登记进注册表 */
    self->current = cfg;
    self->locked  = 0;
    module_set_handler(self->mod, cfg->evt_table, self);
}

void ac_module_start_scan(ac_module_t *self)
{
    if (!self || !self->mod || !self->mod->handler)
        return;

    self->locked = 0;
    if (self->mod->handler->on_scan)
        self->mod->handler->on_scan(self);
}

void ac_module_lock(ac_module_t *self)
{
    if (self) self->locked = 1;
}

int ac_module_locked(ac_module_t *self)
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
