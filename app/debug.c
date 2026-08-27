#include "debug.h"
#include "uart.h"
#include "uart_instance.h"
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
    uart_configure(&uart1, &cfg);
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
