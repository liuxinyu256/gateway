#ifndef DEBUG_MODULE_H
#define DEBUG_MODULE_H
#include "module.h"   /* 框架核心：统一提供 sender/receiver/bus 等抽象 */
#include <stdarg.h>

/* 调试/日志模块：用 module_t 框架跑 UART1 收发，统一输出日志 */
void debug_module_start(void);

/* 公共发送：使用调用方栈上独立缓冲区格式化后发到 UART1 */
void log_printf(const char *fmt, ...);
void log_vprintf(const char *fmt, va_list ap);

/* 调试/日志模块统一管理的 HEX 打印：由 debug 模块负责格式化并发送到 UART1 */
void log_hex_dump(const char *tag, const uint8_t *data, uint16_t len);

/* 日志开关查询：供其他模块决定是否打印 */
uint8_t log_event_enabled(void);
uint8_t log_rx_enabled(void);

/* 系统运行时长：FreeRTOS tick 换算为秒 */
uint32_t debug_uptime_s(void);

#endif /* DEBUG_MODULE_H */
