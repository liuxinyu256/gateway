#ifndef AC_BRAND_H
#define AC_BRAND_H

#include <stdint.h>

/* ---- AC 品牌/协议公共定义 ----
 *
 * 本头只放“品牌协议表”相关类型，不 include 模块框架：
 *   - ac_state_t / ac_protocol_ops_t
 *   - 模式/风速/摆风等枚举
 *   - 能力、物理层、品牌配置
 *   - AC_BRAND_LIST 品牌清单
 *
 * 品牌文件（ac_test.h 等）只依赖本头，不依赖 module/gateway。
 */

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

/* 品牌清单 (单一数据源): 每条 BRAND(枚举名, cfg 变量名), 显式配对
 * 枚举、注册表、extern 声明都由它生成; id 由清单顺序编码, 禁止重排;
 * 0 位是契约占位, 不进清单 */
// clang-format off
#define AC_BRAND_LIST(BRAND) \
    BRAND(ac_test, ac_test_cfg)
// clang-format on

#define AC_BRAND_ENUM(name, cfg) name,
#define AC_BRAND_TABLE(name, cfg) [name] = &cfg,
#define AC_BRAND_EXTERN(name, cfg) extern const ac_brand_config_t cfg;

/* AC 模块/协议自己的状态：只描述 AC 关心的字段，不依赖 gateway_state_t */
typedef struct {
    uint8_t power;
    uint8_t mode;
    uint8_t set_temp;
    uint8_t room_temp;
    uint8_t fan;
    uint8_t swing;
    uint8_t error_code;
} ac_state_t;

/* 协议 TX 回调：品牌只负责往 buf 填帧，返回帧长，不负责发送 */
typedef uint16_t (*ac_tx_builder_t)(uint8_t *buf, uint16_t max);

/* 品牌事件：模块层只认识通用事件，品牌 RX 事件从 AC_EV_RX_BASE 起自定义 */
typedef enum {
    AC_EV_NONE = 0,      /* 无事件/不处理 */
    AC_EV_POLL,          /* 定时/周期驱动 */
    AC_EV_SCAN,          /* 扫描驱动 */
    AC_EV_STATE_SYNC,    /* 网关状态同步/控制 */
    AC_EV_RX_BASE = 0x10 /* 品牌 RX 解析结果从这开始 */
} ac_event_t;

/* 协议 RX 解析：品牌解析到 AC 状态，返回品牌事件号；0=无法识别/不处理 */
typedef uint8_t (*ac_rx_parser_t)(const uint8_t *data, uint16_t len,
                                  ac_state_t *out);

/* 发送状态机：品牌根据事件+当前状态，填要发的帧并给出下次定时 */
typedef uint16_t (*ac_state_machine_t)(uint8_t event,
                                       const ac_state_t *state,
                                       uint8_t *tx, uint16_t tx_max,
                                       uint16_t *next_period_ms);

typedef struct {
    ac_tx_builder_t     on_scan;        /* 扫描/激活帧 */
    ac_state_machine_t  state_machine;  /* 发送状态机：唯一修改协议流程的地方 */
    ac_rx_parser_t      rx_parse;       /* 接收解析：帧 -> 品牌事件 */
    uint16_t            poll_period_ms; /* 该协议默认轮询间隔 */
} ac_protocol_ops_t;

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

/* 物理层类型：品牌声明自己用什么物理层 */
typedef enum {
    AC_PHY_UART = 0,     /* 纯 UART */
    AC_PHY_RS485,        /* UART + RS485 方向控制 */
    AC_PHY_HBS,          /* HBS 总线（预留） */
    AC_PHY_MANCHESTER,   /* 曼彻斯特（预留） */
} ac_phy_type_t;

/* UART 物理层配置（仅 UART 使用） */
typedef struct {
    uint32_t baudrate;
    uint8_t  data_bits;
    uint8_t  stop_bits;
    uint8_t  parity;
    uint16_t receiver_timeout_ticks; /* 帧间隙超时 */
} uart_phy_cfg_t;

/* 物理层配置：通用结构只保存类型 + 私有配置指针 */
typedef struct {
    ac_phy_type_t phy_type;
    const void   *cfg;   /* 指向具体物理层配置（uart_phy_cfg_t / manchester_phy_cfg_t ...） */
} ac_phy_cfg_t;

typedef struct
{
    ac_brand_id_t brand_id; /* 品牌唯一编码 */
    const ac_phy_cfg_t *phy_cfg;   /* 物理层配置 */
    const ac_protocol_ops_t *protocol_ops; /* 协议 ops：品牌只填帧/解析，不碰框架 */
    ac_ability_t ability; /* 品牌能力描述 (静态, 注册时填入) */
} ac_brand_config_t;

/* 品牌配置实例声明：定义在各品牌 .c，由 AC_BRAND_LIST 统一登记 */
AC_BRAND_LIST(AC_BRAND_EXTERN)

/* 品牌注册表: 由清单生成 (定义在 ac_module.c), 按下标 id 索引,
 * 两层 const 住 ROM; 未登记槽位为 NULL */
extern const ac_brand_config_t *const brand_table[AC_BRAND_NUM];

#endif /* AC_BRAND_H */
