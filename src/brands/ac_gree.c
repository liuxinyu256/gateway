/**
 * ac_gree.c —— 格力空调协议 (状态机版)
 *
 * 帧格式: | AA | 01 | CMD | VAL | CHK | 55 |
 *           6字节固定长度
 *           CHK = ~(AA + 01 + CMD + VAL)
 *
 * 协议表:
 *   CMD=0x10  电源: VAL=0关/1开
 *   CMD=0x11  模式: 0x02制冷 0x04制热 0x01送风 0x08除湿 0x00自动
 *   CMD=0x12  风量: 0x01低 0x02中 0x03高 0x00自动
 *   CMD=0x13  摆风: 0x00关 0x0F开
 *   CMD=0x20  查询应答
 *   CMD=0xFF  扫描识别帧
 *
 * 状态机:
 *   B_IDLE → on_periodic发送查询 → B_WAIT_QUERY_RESP
 *   B_IDLE → on_control发送控制帧 → B_WAIT_CTRL_ACK
 *   B_WAIT_QUERY_RESP → on_rx_frame解析应答 → B_IDLE
 *   B_WAIT_CTRL_ACK   → on_rx_frame解析ACK  → B_IDLE
 *   任一等待状态超时  → on_timeout重发/放弃 → B_IDLE
 */

#include "gateway.h"
#include "ac_brand_manager.h"

/* ============================================================
 *  协议映射表: 抽象值 → 帧字节
 * ============================================================ */

/* 模式: [制冷,制热,送风,除湿,自动] */
static const uint8_t mode_tbl[] = { 0x02, 0x04, 0x01, 0x08, 0x00 };

/* 风量: [低,中,高,自动] */
static const uint8_t fan_tbl[]  = { 0x01, 0x02, 0x03, 0x00 };

/* ============================================================
 *  组帧: cmd(0电源/1模式/2风量/3摆风) + val → 6字节帧
 *  返回帧长度, 失败返回0
 * ============================================================ */
static uint8_t gree_build(uint8_t cmd, uint8_t val, uint8_t *f) {
    f[0] = 0xAA;
    f[1] = 0x01;

    switch (cmd) {
    case 0: /* 电源 */
        f[2] = 0x10;
        f[3] = val;
        break;
    case 1: /* 模式 */
        f[2] = 0x11;
        f[3] = mode_tbl[val];
        break;
    case 2: /* 风量 */
        f[2] = 0x12;
        f[3] = fan_tbl[val];
        break;
    case 3: /* 摆风 */
        f[2] = 0x13;
        f[3] = val ? 0x0F : 0x00;
        break;
    default:
        return 0;
    }

    /* 校验和: ~(字节0+1+2+3) */
    uint8_t s = 0;
    for (int i = 0; i < 4; i++) {
        s += f[i];
    }
    f[4] = ~s;
    f[5] = 0x55;

    return 6;
}

/* ============================================================
 *  品牌实例
 * ============================================================ */

static brand_t g_gree = {
    .dev_type  = DEV_AC,
    .state     = B_IDLE,
    .max_retry = 3,
};

/* 扫描期标志: 扫描中发FF识别帧, 锁定后恢复正常查询 */
static uint8_t g_scanning = 1;

/* ============================================================
 *  ISR: 每收到一个字节, 喂入模块共享封包器
 *  由品牌管理器在品牌切换时绑定到 phy 的 RX 回调
 * ============================================================ */
static void on_rx_byte(uint8_t byte, void *ctx) {
    brand_t *b = (brand_t *)ctx;
    receiver_put_byte(b->mod->pkt, byte);
}

/* ============================================================
 *  send_task: 定时轮询 / 心跳
 *  扫描期: 发送 FF 识别帧
 *  锁定后: 发送 0x20 查询帧, 转为等待应答状态
 * ============================================================ */
static void on_periodic(void *ctx) {
    brand_t *b = (brand_t *)ctx;

    /* 正在等待上次的应答, 跳过本轮 */
    if (b->state != B_IDLE) {
        return;
    }

    uint8_t f[6];

    if (g_scanning) {
        /* 扫描期: 发送品牌识别帧 */
        uint8_t sf[] = { 0xAA, 0x01, 0xFF, 0x00, 0x00, 0x55 };
        sender_send(&b->mod->sender, sf, 6);
    } else {
        /* 锁定后: 发送状态查询帧 */
        gree_build(0, 1, f);
        sender_send(&b->mod->sender, f, 6);

        /* 状态转移: 等待查询应答 */
        b->state      = B_WAIT_QUERY_RESP;
        b->timeout_at = xTaskGetTickCount() + pdMS_TO_TICKS(500);
        b->retry      = 0;
    }
}

/* ============================================================
 *  rx_task: 解析收到的帧
 *  根据当前状态决定如何处理帧内容
 * ============================================================ */
static int on_rx_frame(void *ctx, uint8_t *d, uint16_t n) {
    brand_t *b = (brand_t *)ctx;

    /* 帧长度和头尾校验 */
    if (n < 6 || d[0] != 0xAA || d[5] != 0x55) {
        return 0;  /* 不是格力的帧 */
    }

    switch (b->state) {

    case B_IDLE:
        /* 空闲状态收到帧: 可能是扫描应答 */
        if (g_scanning && d[2] == 0xFF) {
            /* 扫描成功, 锁定品牌 */
            g_scanning = 0;
            b->state   = B_IDLE;
            ac_brand_manager_lock();
            ac_brand_manager_set_poll_period(5000);  /* 锁定后降速轮询 */
            return 1;
        }
        return 0;

    case B_WAIT_QUERY_RESP:
        /* 等待查询应答: 解析状态数据 */
        if (d[2] == 0x20) {
            gateway_state_t s;
            gateway_state_get(&s);

            s.power    = d[3] >> 7;       /* bit7=电源 */
            s.mode     = d[3] & 0x0F;      /* bit0-3=模式 */
            s.set_temp = d[4] >> 1;        /* bit1-7=设定温度 */

            gateway_state_update(&s);  /* 更新全局状态, 自动广播 */
            b->state = B_IDLE;
            return 1;
        }
        return 0;

    case B_WAIT_CTRL_ACK:
        /* 等待控制ACK: 检查是否是之前发过的控制命令的应答 */
        if (d[2] >= 0x10 && d[2] <= 0x13) {
            b->state = B_IDLE;
            return 1;
        }
        return 0;

    default:
        return 0;
    }
}

/* ============================================================
 *  send_task: 收到外部控制命令 (BLE/米家/涂鸦 → 网关 → 格力)
 *  组控制帧发出, 转为等待ACK状态
 * ============================================================ */
static void on_control(void *ctx, uint8_t cmd, uint8_t val) {
    brand_t *b = (brand_t *)ctx;

    /* 正在等待上次操作的应答, 拒绝新命令 */
    if (b->state != B_IDLE) {
        return;
    }

    uint8_t f[8];
    uint8_t n = gree_build(cmd, val, f);
    if (n == 0) {
        return;  /* 无效命令 */
    }

    sender_send(&b->mod->sender, f, n);

    /* 状态转移: 等待空调确认 */
    b->state      = B_WAIT_CTRL_ACK;
    b->timeout_at = xTaskGetTickCount() + pdMS_TO_TICKS(500);
    b->retry      = 0;
}

/* ============================================================
 *  send_task: 超时处理
 *  未超过重试次数 → 重发上次的查询帧
 *  超过 → 放弃, 回到空闲
 * ============================================================ */
static void on_timeout(void *ctx) {
    brand_t *b = (brand_t *)ctx;

    /* 空闲状态没有超时 */
    if (b->state == B_IDLE) {
        return;
    }

    if (b->retry < b->max_retry) {
        /* 重试: 发送查询帧 */
        b->retry++;
        uint8_t f[] = { 0xAA, 0x01, 0x20, 0x00, 0xDE, 0x55 };
        sender_send(&b->mod->sender, f, 6);
        b->timeout_at = xTaskGetTickCount() + pdMS_TO_TICKS(500);
    } else {
        /* 放弃, 下轮轮询重新开始 */
        b->state = B_IDLE;
    }
}

/* ============================================================
 *  send_task: 上电扫描
 *  进入扫描模式, 下轮 on_periodic 发识别帧
 * ============================================================ */
static void on_scan(void *ctx) {
    brand_t *b = (brand_t *)ctx;
    g_scanning = 1;
    b->state   = B_IDLE;
}

/* ============================================================
 *  事件表: 所有 handler 必须立即返回, 不做任何等待
 * ============================================================ */
static const event_handler_t gree_table = {
    .on_rx_byte       = on_rx_byte,        /* ISR: 喂封包器 */
    .on_periodic_send = on_periodic,       /* send_task: 轮询/心跳 */
    .on_rx_frame      = on_rx_frame,       /* rx_task: 解析应答 */
    .on_rx_isr        = NULL,              /* ISR: 格力无紧急ACK需求 */
    .on_control_cmd   = on_control,        /* send_task: 控制命令 */
    .on_need_ack      = NULL,              /* send_task: 格力无握手需求 */
    .on_scan          = on_scan,           /* send_task: 上电扫描 */
    .on_timeout       = on_timeout,        /* send_task: 超时重试 */
};

/* ============================================================
 *  品牌配置: 注册到品牌管理器
 * ============================================================ */
const brand_config_t gree_brand = {
    .brand_id  = BRAND_ID_GREE,
    .dev_type  = DEV_AC,
    .evt_table = &gree_table,
    .phy                    = { .type = PHY_RS485_8N1, .baudrate = 9600 },
    .receiver_timeout_ticks = 5,
};
