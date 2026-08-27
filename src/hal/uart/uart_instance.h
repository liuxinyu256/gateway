#ifndef UART_INSTANCE_H
#define UART_INSTANCE_H
#include "uart.h"

/**
 * 4 个 UART 全局实例
 * uart0 / uart1 / uart2 / uart3
 *
 * ops 由具体芯片初始化时挂上，例如：
 *   uart0.ops = &ch579_uart_ops;
 */

extern uart_t uart0;
extern uart_t uart1;
extern uart_t uart2;
extern uart_t uart3;

uart_t *uart_get(uint8_t id);

#endif
