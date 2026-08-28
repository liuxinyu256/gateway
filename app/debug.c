#include "debug.h"
#include "uart.h"
#include "uart_instance.h"
#ifdef __CH579__
#include "CH57x_common.h"
#endif
#include <stdarg.h>
#include <stdio.h>
#include <string.h>

static SemaphoreHandle_t debug_mutex;

void debug_init(void)
{
    debug_mutex = xSemaphoreCreateMutex();
    uart_cfg_t cfg = {
        .baudrate  = 115200,
        .data_bits = 8,
        .stop_bits = 1,
        .parity    = 0,
    };
#ifdef __CH579__
    SetSysClock(CLK_SOURCE_PLL_32MHz);
    DelayMs(1);
    /* UART1 默认 PA8(RX)/PA9(TX) */
    GPIOA_ModeCfg(GPIO_Pin_8, GPIO_ModeIN_PU);
    GPIOA_ModeCfg(GPIO_Pin_9, GPIO_ModeOut_PP_5mA);
    GPIOA_SetBits(GPIO_Pin_9);
#endif
    uart_configure(&uart1, &cfg);
    uart_irq_tx_disable(&uart1);
    uart_irq_rx_disable(&uart1);
#ifdef __CH579__
    UART1_CLR_TXFIFO();
    UART1_CLR_RXFIFO();
#endif
}

void debug_putc(char c)
{
    if (c == '\n') {
        while (!uart_irq_tx_ready(&uart1)) { }
        uart_write(&uart1, (uint8_t)'\r');
    }
    while (!uart_irq_tx_ready(&uart1)) { }
    uart_write(&uart1, (uint8_t)c);
}

void debug_puts(const char *s)
{
    if (!s) return;

    if (debug_mutex)
        xSemaphoreTake(debug_mutex, portMAX_DELAY);

    while (*s)
        debug_putc(*s++);

    if (debug_mutex)
        xSemaphoreGive(debug_mutex);
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
