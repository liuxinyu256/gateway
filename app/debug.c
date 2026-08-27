#include "debug.h"
#include "uart.h"
#include "uart_instance.h"
#ifdef __CH579__
#include "CH57x_common.h"
#endif
#include <stdarg.h>
#include <stdio.h>
#include <string.h>

void debug_init(void)
{
    uart_cfg_t cfg = {
        .baudrate  = 115200,
        .data_bits = 8,
        .stop_bits = 1,
        .parity    = 0,
    };
#ifdef __CH579__
    SetSysClock(CLK_SOURCE_PLL_32MHz);
    /* UART1 默认 PA8(RX)/PA9(TX) */
    GPIOA_ModeCfg(GPIO_Pin_8, GPIO_ModeIN_PU);
    GPIOA_ModeCfg(GPIO_Pin_9, GPIO_ModeOut_PP_5mA);
#endif
    uart_configure(&uart1, &cfg);
    uart_irq_tx_disable(&uart1);
    uart_irq_rx_disable(&uart1);
}

void debug_putc(char c)
{
    if (c == '\n') {
        uart_write(&uart1, (uint8_t)'\r');
    }
    uart_write(&uart1, (uint8_t)c);
}

void debug_puts(const char *s)
{
    if (!s) return;
    while (*s)
        debug_putc(*s++);
}

void debug_printf(const char *fmt, ...)
{
    char buf[128];
    va_list ap;

    va_start(ap, fmt);
    vsnprintf(buf, sizeof(buf), fmt, ap);
    va_end(ap);

    debug_puts(buf);
}
