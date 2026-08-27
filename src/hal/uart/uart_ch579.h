#ifndef UART_CH579_H
#define UART_CH579_H
#include "uart.h"

#ifdef __CH579__

/* CH579 私有数据：每个 UART 实例保存自己的编号 */
typedef struct {
    uint8_t id;
} uart_ch579_drv_t;

/* 4 个 UART 共用这一张 ops 表 */
extern const uart_ops_t ch579_uart_ops;

/* UART 中断总入口：UART0/1/2/3 ISR 里调用 */
void ch579_uart_irq_handler(uint8_t id);

#endif

#endif
