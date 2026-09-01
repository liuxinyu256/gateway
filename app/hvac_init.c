/**
 * hvac_init.c —— 网关初始化
 *
 * 上层负责装配并启动：
 *   - bsp/rs485 板级初始化
 *   - ac_phy_init 装配编码器/解码器/发送器/接收器
 *   - 注册品牌并启动模块
 *   - 启动调试模块
 */

#include "hvac_init.h"
#include "gateway.h"
#include "ac_module.h"
#include "ac_test.h"
#include "ac_phy.h"
#include "debug_module.h"
#include "rs485.h"
#include "bsp.h"

#ifdef __CH579__
/* RS485 方向回调适配：bus 层调用 (tx, ctx)，转给 rs485 HAL */
static void hvac_rs485_dir(uint8_t tx, void *ctx)
{
    rs485_set_dir((rs485_t *)ctx, tx);
}
#endif

static ac_module_t   g_ac = { .base.ops = &ac_module_ops };
static ac_io_t       g_ac_io;

/* 1. AC 物理层装配：由品牌 phy_cfg 决定（内部包含 RS485 方向控制） */
static uint8_t init_ac_phy(void)
{
    if (ac_phy_init(ac_test_cfg.phy_cfg, &g_ac.base.bus, &g_ac_io) != 0)
        return 1;

    g_ac.base.sender   = g_ac_io.sender;
    g_ac.base.receiver = g_ac_io.receiver;
    return 0;
}

/* 3. AC 模块初始化 + 品牌注册 + 启动 */
static void init_ac_module(void)
{
    ac_init_cfg_t cfg = {
        .baudrate    = 9600,
        .brand_table = brand_table,
        .brand_count = AC_BRAND_NUM,
    };

    module_init(&g_ac.base, &cfg);

#ifdef __CH579__
    /* 必须在 module_init 之后设置：module_base_init 会 bus_init 清零 */
    bus_set_rs485_enable(&g_ac.base.bus, 1);
    bus_set_dir_callback(&g_ac.base.bus, hvac_rs485_dir, g_ac_io.rs485);
#endif

    ac_module_register(&g_ac, &ac_test_cfg);
    gateway_set_module(0, &g_ac.base);

    module_start(&g_ac.base);
    module_set_poll_period(&g_ac.base, 1000);   /* 测试：1s 周期发读请求 */
    ac_module_start_scan(&g_ac);
}

void hvac_start(void)
{
    gateway_init();

    bsp_board_init();       /* 板级外围电路选择 */
    if (init_ac_phy() != 0) /* 物理层装配（含 RS485） */
        return;

    init_ac_module();       /* AC 模块初始化/注册/启动 */
    debug_module_start();   /* 调试模块 */
}
