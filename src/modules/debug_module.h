#ifndef DEBUG_MODULE_H
#define DEBUG_MODULE_H
#include "module.h"   /* 框架核心：统一提供 sender/receiver/bus 等抽象 */
#include <stdarg.h>

/* 调试模块：用 module_t 框架跑 UART1 收发，验证收发链路 */
void debug_module_start(void);

/* 公共发送：使用调用方栈上独立缓冲区格式化后发到 UART1 */
void debug_printf(const char *fmt, ...);
void debug_vprintf(const char *fmt, va_list ap);

/* 调试模块统一管理的 HEX 打印：由 debug 模块负责格式化并发送到 UART1 */
void debug_hex_dump(const char *tag, const uint8_t *data, uint16_t len);

#endif
