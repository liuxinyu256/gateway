#ifndef DEBUG_MODULE_H
#define DEBUG_MODULE_H
#include "module.h"   /* 框架核心：统一提供 sender/receiver/bus 等抽象 */
#include <stdarg.h>

/* 调试模块：用 module_t 框架跑 UART1 收发，验证收发链路 */
void debug_module_start(void);

/* 公共发送：复用调试模块 tx_buf，发到 UART1 */
void debug_printf(const char *fmt, ...);
void debug_vprintf(const char *fmt, va_list ap);

#endif
