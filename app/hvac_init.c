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
#include "ble_module.h"
#include "bsp.h"

static ac_module_t   g_ac = { .base.ops = &ac_module_ops };
static ac_io_t       g_ac_io;

/* 0. AC 模块基础初始化：先建好 bus/队列，物理层再往 bus 上装配方向控制 */
static uint8_t init_ac_module_base(void)
{
    ac_init_cfg_t cfg = {
        .baudrate    = 9600,
        .brand_table = brand_table,
        .brand_count = AC_BRAND_NUM,
    };

    return module_init(&g_ac.base, &cfg);
}

/* 1. AC 物理层装配：由品牌 phy_cfg 决定（内部包含 RS485 方向控制） */
static uint8_t init_ac_phy(void)
{
    if (ac_phy_init(ac_test_cfg.phy_cfg, &g_ac.base.bus, &g_ac_io) != 0)
        return 1;

    g_ac.base.sender   = g_ac_io.sender;
    g_ac.base.receiver = g_ac_io.receiver;
    module_set_bus_type(&g_ac.base, MODULE_BUS_SERIAL);
    return 0;
}

/* 3. AC 模块注册 + 启动 */
static void init_ac_module(void)
{
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

    /* 顺序：先 module_init（内部 bus_init 会清零 bus），
     * 再 ac_phy_init 装配物理层，此时方向控制写进 bus 不会被清掉 */
    if (init_ac_module_base() != 0)
        return;
    if (init_ac_phy() != 0) /* 物理层装配（含 RS485 方向控制） */
        return;

    init_ac_module();       /* AC 模块注册/启动 */
    debug_module_start();   /* 调试模块 */
    /* BLE 模块暂不启动，避免额外任务影响当前稳定性 */
    /* ble_module_start(); */
}
