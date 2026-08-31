/**
 * ac_test.h —— 测试品牌
 *
 * 用于 AC 模块基本功能测试：
 *   - 定时发送 Modbus 读请求
 *   - 收到响应打印到 UART1
 */
#ifndef AC_TEST_H
#define AC_TEST_H
#include "ac_module.h"

extern const ac_brand_config_t ac_test_cfg;

#endif /* AC_TEST_H */
