#include "uart_instance.h"
#include <stddef.h>

#ifdef __CH579__
#include "uart_ch579.h"

static uart_ch579_drv_t drv0 = { .id = 0 };
static uart_ch579_drv_t drv1 = { .id = 1 };
static uart_ch579_drv_t drv2 = { .id = 2 };
static uart_ch579_drv_t drv3 = { .id = 3 };

uart_t uart0 = { .ops = &ch579_uart_ops, .drv = &drv0 };
uart_t uart1 = { .ops = &ch579_uart_ops, .drv = &drv1 };
uart_t uart2 = { .ops = &ch579_uart_ops, .drv = &drv2 };
uart_t uart3 = { .ops = &ch579_uart_ops, .drv = &drv3 };
#else
typedef struct {
    uint8_t id;
} uart_drv_t;

static uart_drv_t drv0 = { .id = 0 };
static uart_drv_t drv1 = { .id = 1 };
static uart_drv_t drv2 = { .id = 2 };
static uart_drv_t drv3 = { .id = 3 };

uart_t uart0 = { .drv = &drv0 };
uart_t uart1 = { .drv = &drv1 };
uart_t uart2 = { .drv = &drv2 };
uart_t uart3 = { .drv = &drv3 };
#endif

uart_t *uart_get(uint8_t id)
{
    switch (id) {
    case 0: return &uart0;
    case 1: return &uart1;
    case 2: return &uart2;
    case 3: return &uart3;
    default: return NULL;
    }
}
