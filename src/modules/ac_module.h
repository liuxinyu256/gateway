#ifndef AC_MODULE_H
#define AC_MODULE_H
#include "module.h"
#include "gateway_device.h"
#include "receiver_timeout.h"

/* ---- ac品牌基类 ---- */

typedef enum
{
    DEV_AC,
    DEV_FRESH_AIR,

} device_type_t;

typedef enum
{
    B_IDLE,
    B_WAIT_QUERY_RESP,
    B_WAIT_CTRL_ACK,
    B_WAIT_HANDSHAKE,
    B_BRAND_CUSTOM = 0x10,
} brand_state_t;

/* 品牌清单 (单一数据源): 每条 BRAND(枚举名, cfg 指针), 显式配对
 * 枚举与注册表都由它生成; id 由清单顺序编码, 禁止重排;
 * 0 位是契约占位, 不进清单 */
// clang-format off
#define AC_BRAND_LIST(BRAND)
// clang-format on

#define AC_BRAND_ENUM(name, cfg) name,
#define AC_BRAND_TABLE(name, cfg) [name] = cfg,

/* ---- 品牌配置和实例 ---- */
// clang-format off
typedef enum
{
    null_0 = 0, /* 0 位契约占位 (无 cfg, 不进清单) */
    AC_BRAND_LIST(AC_BRAND_ENUM)
    AC_BRAND_NUM
} ac_brand_id_t; // 这里唯一编码品牌
// clang-format on

/* ---- 枚举: 值即位索引, NUM 是哨兵 (数量/遍历) ---- */
typedef enum
{
    MODE_COOL = 0,
    MODE_HEAT = 1,
    MODE_FAN = 2,
    MODE_DRY = 3,
    MODE_AUTO = 4,
    MODE_NUM,
} mode_t;

typedef enum
{
    FAN_AUTO = 0, /* 自动 */
    FAN_1 = 1,    /* 1 档 */
    FAN_2 = 2,
    FAN_3 = 3,
    FAN_4 = 4,
    FAN_5 = 5,
    FAN_6 = 6, /* 6 档 */
    FAN_NUM,
} fan_speed_t;

typedef enum
{
    SWING_OFF = 0,
    SWING_UD = 1,
    SWING_LR = 2,
    SWING_ALL = 3,
    SWING_NUM, /* 哨兵 */
} swing_t;

/* 扩展功能位 */
#define AC_FEAT_TIMER (1u << 0)  /* 定时 */
#define AC_FEAT_SLEEP (1u << 1)  /* 睡眠 */
#define AC_FEAT_HEALTH (1u << 2) /* 健康 */

/* ---- AC能力描述: 枚举值即位索引, 能力 = 位掩码 ---- */
typedef struct
{
    uint16_t mode_caps;  /* 支持的模式: 1<<MODE_COOL | ... */
    uint16_t fan_caps;   /* 支持的风速: 1<<FAN_LOW | ... */
    uint16_t swing_caps; /* 支持的摆风 */
    uint8_t temp_min;    /* 设定温度下限 */
    uint8_t temp_max;    /* 设定温度上限 */
    uint8_t temp_step;   /* 步进: 10=1℃, 5=0.5℃ */
    uint16_t features;   /* 扩展功能位: AC_FEAT_* */
} ac_ability_t;

typedef struct
{
    ac_brand_id_t brand_id; /* 品牌唯一编码 */
    const event_handler_t *evt_table;
    uint16_t receiver_timeout_ticks;
    ac_ability_t ability; /* 品牌能力描述 (静态, 注册时填入) */
} ac_brand_config_t;

/* 品牌注册表: 由清单生成 (定义在 ac_module.c), 按下标 id 索引,
 * 两层 const 住 ROM; 未登记槽位为 NULL */
extern const ac_brand_config_t *const brand_table[AC_BRAND_NUM];
/* ---- AC 模块 ---- */
typedef struct
{
    module_t base;                               /* ac_module_t 自己就是 module_t 的子类 */
    gateway_state_t ac_state;                    /* AC 模块的完整状态 */
    const ac_brand_config_t *const *brand_table; /* 品牌注册表地址 (init 传入) */
    uint8_t brand_count;                         /* 注册表长度 */
    const ac_brand_config_t *current;            /* 当前激活品牌 */
    uint8_t locked;                              /* 品牌锁定标志: 0=扫描中, 1=已锁定 */
} ac_module_t;

/* AC 模块初始化参数 (通过 module_init 的 cfg 传入) */
typedef struct {
    uint32_t baudrate;
    const ac_brand_config_t *const *brand_table;
    uint8_t brand_count;
} ac_init_cfg_t;

extern const module_ops_t ac_module_ops;

/* brand_table/brand_count: 品牌注册表地址与长度 (编译期静态表)
 * sender / receiver 都由上层指针注入，模块不持有具体实现 */
void ac_module_init(ac_module_t *self,
                    const ac_brand_config_t *const *brand_table,
                    uint8_t brand_count);
void ac_module_register(ac_module_t *self, const ac_brand_config_t *cfg);
void ac_module_start_scan(ac_module_t *self);
void ac_module_lock(ac_module_t *self);
uint8_t ac_module_locked(ac_module_t *self);
const ac_brand_config_t *ac_module_current(ac_module_t *self);
void ac_module_set_poll_period(ac_module_t *self, uint16_t period_ms);

/* 状态上报：AC 模块状态变化后同步到网关 */
void ac_module_publish_state(ac_module_t *self);
/* 更新 AC 模块状态：new_state 必须是“读当前完整状态 → 改支持字段”后的完整状态 */
void ac_module_update_state(ac_module_t *self, const gateway_state_t *new_state);
#endif
