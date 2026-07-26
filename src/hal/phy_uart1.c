/**
 * phy_uart1.c —— CH579 UART1 RS485 物理层驱动
 */

#include "phy_uart1.h"
#include "bus.h"

#ifdef __CH579__
#include "CH57x_common.h"
#endif

typedef struct {
    phy_driver_t      base;
    void             (*rx_callback)(uint8_t byte, void *ctx);
    void              *rx_callback_ctx;
    sender_t     *tx;
} phy_uart1_t;

#define UART1_POOL_MAX 1
static phy_uart1_t pool[UART1_POOL_MAX];
static uint8_t     pool_used;

static void uart1_forward_received_byte(phy_driver_t *p, uint8_t byte) {
    phy_uart1_t *up = (phy_uart1_t *)p;
    if (!up->base.sending && up->rx_callback)
        up->rx_callback(byte, up->rx_callback_ctx);
}

static void uart1_set_receive_callback(phy_driver_t *p,
                                        void (*cb)(uint8_t, void*), void *ctx) {
    phy_uart1_t *up = (phy_uart1_t *)p;
    up->rx_callback     = cb;
    up->rx_callback_ctx = ctx;
}

#ifdef __CH579__

static int uart1_open(phy_driver_t *p) {
    (void)p;
    GPIOA_SetBits(GPIO_Pin_9);
    GPIOA_ModeCfg(GPIO_Pin_8, GPIO_ModeIN_PU);
    GPIOA_ModeCfg(GPIO_Pin_9, GPIO_ModeOut_PP_5mA);
    UART1_DefInit();
    UART1_BaudRateCfg(115200);
    UART1_ByteTrigCfg(UART_1BYTE_TRIG);
    UART1_INTCfg(ENABLE, RB_IER_RECV_RDY | RB_IER_LINE_STAT);
    NVIC_SetPriority(UART1_IRQn, 1);
    NVIC_EnableIRQ(UART1_IRQn);
    return 0;
}

static void uart1_close(phy_driver_t *p) {
    (void)p;
    UART1_INTCfg(DISABLE, RB_IER_RECV_RDY | RB_IER_THR_EMPTY | RB_IER_LINE_STAT);
}

static void uart1_write(phy_driver_t *p, uint8_t byte) {
    p->sending = 1;
    R8_UART1_THR = byte;
    UART1_INTCfg(ENABLE, RB_IER_THR_EMPTY);
}

void UART1_IRQHandler(void) {
    phy_uart1_t *up = &pool[0];
    phy_driver_t *p = &up->base;
    uint8_t b;

    switch (UART1_GetITFlag()) {
    case UART_II_RECV_RDY:
        b = UART1_RecvByte();
        p->forward_received_byte(p, b);
        break;
    case UART_II_RECV_TOUT:
        while (UART1_GetLinSTA() & STA_RECV_DATA) {
            b = UART1_RecvByte();
            p->forward_received_byte(p, b);
        }
        break;
    case UART_II_THR_EMPTY:
        UART1_INTCfg(DISABLE, RB_IER_THR_EMPTY);
        if (up->tx) sender_on_thr_empty(up->tx);
        break;
    default: break;
    }
}

#else

static int  uart1_open(phy_driver_t *p)  { (void)p; return 0; }
static void uart1_close(phy_driver_t *p) { (void)p; }
static void uart1_write(phy_driver_t *p, uint8_t b) { (void)p; (void)b; }

#endif

phy_driver_t *phy_uart1_create(sender_t *tx) {
    if (pool_used >= UART1_POOL_MAX) return NULL;
    phy_uart1_t *up = &pool[pool_used++];

    up->base.open                  = uart1_open;
    up->base.close                 = uart1_close;
    up->base.write                 = uart1_write;
    up->base.forward_received_byte = uart1_forward_received_byte;
    up->base.set_receive_callback  = uart1_set_receive_callback;
    up->base.half_duplex           = 1;
    up->base.sending               = 0;
    up->tx = tx;

    return &up->base;
}

void phy_uart1_set_sender(phy_driver_t *phy, sender_t *tx) {
    if (!phy) return;
    phy_uart1_t *up = (phy_uart1_t *)phy;
    up->tx = tx;
}
