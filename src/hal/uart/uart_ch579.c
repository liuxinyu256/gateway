#include "uart_ch579.h"
#include "uart_instance.h"

#ifdef __CH579__
#include "CH57x_common.h"
#include "gpio.h"
#include "gpio_instance.h"

/* ---- 公共工具 ---- */

static uint8_t ch579_lcr(const uart_cfg_t *cfg)
{
    uint8_t lcr = 3; /* 默认 8 位 */

    if (cfg && cfg->data_bits >= 5 && cfg->data_bits <= 8)
        lcr = cfg->data_bits - 5;

    if (cfg && cfg->stop_bits == 2)
        lcr |= RB_LCR_STOP_BIT;

    if (cfg) {
        if (cfg->parity == 1)
            lcr |= RB_LCR_PAR_EN;              /* 奇校验 */
        else if (cfg->parity == 2)
            lcr |= RB_LCR_PAR_EN | (1u << 4);  /* 偶校验 */
    }

    return lcr;
}

static uint8_t ch579_lsr(uint8_t id)
{
    switch (id) {
    case 0: return UART0_GetLinSTA();
    case 1: return UART1_GetLinSTA();
    case 2: return UART2_GetLinSTA();
    case 3: return UART3_GetLinSTA();
    default: return 0;
    }
}

static uint8_t ch579_id(uart_t *u)
{
    uart_ch579_drv_t *drv = (uart_ch579_drv_t *)u->drv;
    return drv ? drv->id : 0xFF;
}

static void ch579_nvic_enable(uint8_t id)
{
    switch (id) {
    case 0: NVIC_SetPriority(UART0_IRQn, 1); NVIC_EnableIRQ(UART0_IRQn); break;
    case 1: NVIC_SetPriority(UART1_IRQn, 1); NVIC_EnableIRQ(UART1_IRQn); break;
    case 2: NVIC_SetPriority(UART2_IRQn, 1); NVIC_EnableIRQ(UART2_IRQn); break;
    case 3: NVIC_SetPriority(UART3_IRQn, 1); NVIC_EnableIRQ(UART3_IRQn); break;
    default: break;
    }
}

/* ---- 统一 ops 实现（通过 u->drv->id 区分 UART） ---- */

/* 通过 GPIO HAL 配置一个引脚 */
static void ch579_gpio_cfg(uint8_t port, uint8_t pin_idx,
                           uint8_t mode, gpio_level_t init_level)
{
    gpio_t *g = gpio_get(port, pin_idx);
    gpio_cfg_t cfg;

    if (!g)
        return;

    cfg.port       = port;
    cfg.pin        = (uint32_t)(1u << pin_idx);
    cfg.mode       = mode;
    cfg.init_level = init_level;
    gpio_init(g, &cfg);
}

/* 根据 UART 编号配置默认引脚 */
static void ch579_gpio_init(uint8_t id)
{
    switch (id) {
    case 0: /* UART0: RX=PB4, TX=PB7 */
        ch579_gpio_cfg(GPIO_PORT_B, 4, GPIO_MODE_INPUT_PULLUP, GPIO_LEVEL_LOW);
        ch579_gpio_cfg(GPIO_PORT_B, 7, GPIO_MODE_OUTPUT_PP,    GPIO_LEVEL_HIGH);
        break;
    case 1: /* UART1: RX=PA8, TX=PA9 */
        ch579_gpio_cfg(GPIO_PORT_A, 8, GPIO_MODE_INPUT_PULLUP, GPIO_LEVEL_LOW);
        ch579_gpio_cfg(GPIO_PORT_A, 9, GPIO_MODE_OUTPUT_PP,    GPIO_LEVEL_HIGH);
        break;
    case 2: /* UART2: RX=PA6, TX=PA7 */
        ch579_gpio_cfg(GPIO_PORT_A, 6, GPIO_MODE_INPUT_PULLUP, GPIO_LEVEL_LOW);
        ch579_gpio_cfg(GPIO_PORT_A, 7, GPIO_MODE_OUTPUT_PP,    GPIO_LEVEL_HIGH);
        break;
    case 3: /* UART3: RX=PA4, TX=PA5 */
        ch579_gpio_cfg(GPIO_PORT_A, 4, GPIO_MODE_INPUT_PULLUP, GPIO_LEVEL_LOW);
        ch579_gpio_cfg(GPIO_PORT_A, 5, GPIO_MODE_OUTPUT_PP,    GPIO_LEVEL_HIGH);
        break;
    default:
        break;
    }
}

static int ch579_configure(uart_t *u, const uart_cfg_t *cfg)
{
    uint8_t id;
    if (!u || !cfg) return -1;

    id = ch579_id(u);
    ch579_gpio_init(id);
    switch (id) {
    case 0:
        UART0_Reset(); UART0_DefInit(); UART0_BaudRateCfg(cfg->baudrate);
        UART0_ByteTrigCfg(UART_1BYTE_TRIG); R8_UART0_LCR = ch579_lcr(cfg);
        break;
    case 1:
        UART1_Reset(); UART1_DefInit(); UART1_BaudRateCfg(cfg->baudrate);
        UART1_ByteTrigCfg(UART_1BYTE_TRIG); R8_UART1_LCR = ch579_lcr(cfg);
        break;
    case 2:
        UART2_Reset(); UART2_DefInit(); UART2_BaudRateCfg(cfg->baudrate);
        UART2_ByteTrigCfg(UART_1BYTE_TRIG); R8_UART2_LCR = ch579_lcr(cfg);
        break;
    case 3:
        UART3_Reset(); UART3_DefInit(); UART3_BaudRateCfg(cfg->baudrate);
        UART3_ByteTrigCfg(UART_1BYTE_TRIG); R8_UART3_LCR = ch579_lcr(cfg);
        break;
    default:
        return -1;
    }
    return 0;
}

static int ch579_read(uart_t *u, uint8_t *byte)
{
    uint8_t id;
    if (!u || !byte) return -1;

    id = ch579_id(u);
    if (!(ch579_lsr(id) & STA_RECV_DATA)) return -1;

    switch (id) {
    case 0: *byte = UART0_RecvByte(); break;
    case 1: *byte = UART1_RecvByte(); break;
    case 2: *byte = UART2_RecvByte(); break;
    case 3: *byte = UART3_RecvByte(); break;
    default: return -1;
    }
    return 0;
}

static int ch579_write(uart_t *u, uint8_t byte)
{
    uint8_t id;
    if (!u) return -1;

    id = ch579_id(u);
    switch (id) {
    case 0:
        if (R8_UART0_TFC == UART_FIFO_SIZE) return -1;
        UART0_SendByte(byte);
        break;
    case 1:
        if (R8_UART1_TFC == UART_FIFO_SIZE) return -1;
        UART1_SendByte(byte);
        break;
    case 2:
        if (R8_UART2_TFC == UART_FIFO_SIZE) return -1;
        UART2_SendByte(byte);
        break;
    case 3:
        if (R8_UART3_TFC == UART_FIFO_SIZE) return -1;
        UART3_SendByte(byte);
        break;
    default:
        return -1;
    }
    return 0;
}

static void ch579_irq_rx_enable(uart_t *u)
{
    uint8_t id = ch579_id(u);
    ch579_nvic_enable(id);
    switch (id) {
    case 0: UART0_INTCfg(ENABLE, RB_IER_RECV_RDY); break;
    case 1: UART1_INTCfg(ENABLE, RB_IER_RECV_RDY); break;
    case 2: UART2_INTCfg(ENABLE, RB_IER_RECV_RDY); break;
    case 3: UART3_INTCfg(ENABLE, RB_IER_RECV_RDY); break;
    default: break;
    }
}

static void ch579_irq_rx_disable(uart_t *u)
{
    uint8_t id = ch579_id(u);
    switch (id) {
    case 0: UART0_INTCfg(DISABLE, RB_IER_RECV_RDY); break;
    case 1: UART1_INTCfg(DISABLE, RB_IER_RECV_RDY); break;
    case 2: UART2_INTCfg(DISABLE, RB_IER_RECV_RDY); break;
    case 3: UART3_INTCfg(DISABLE, RB_IER_RECV_RDY); break;
    default: break;
    }
}

static int ch579_irq_rx_ready(uart_t *u)
{
    uint8_t id = ch579_id(u);
    return (ch579_lsr(id) & STA_RECV_DATA) ? 1 : 0;
}

static void ch579_irq_tx_enable(uart_t *u)
{
    uint8_t id = ch579_id(u);
    ch579_nvic_enable(id);
    switch (id) {
    case 0: UART0_INTCfg(ENABLE, RB_IER_THR_EMPTY); break;
    case 1: UART1_INTCfg(ENABLE, RB_IER_THR_EMPTY); break;
    case 2: UART2_INTCfg(ENABLE, RB_IER_THR_EMPTY); break;
    case 3: UART3_INTCfg(ENABLE, RB_IER_THR_EMPTY); break;
    default: break;
    }
}

static void ch579_irq_tx_disable(uart_t *u)
{
    uint8_t id = ch579_id(u);
    switch (id) {
    case 0: UART0_INTCfg(DISABLE, RB_IER_THR_EMPTY); break;
    case 1: UART1_INTCfg(DISABLE, RB_IER_THR_EMPTY); break;
    case 2: UART2_INTCfg(DISABLE, RB_IER_THR_EMPTY); break;
    case 3: UART3_INTCfg(DISABLE, RB_IER_THR_EMPTY); break;
    default: break;
    }
}

static int ch579_irq_tx_ready(uart_t *u)
{
    uint8_t id = ch579_id(u);
    return (ch579_lsr(id) & STA_TXFIFO_EMP) ? 1 : 0;
}

static int ch579_irq_tx_complete(uart_t *u)
{
    uint8_t id = ch579_id(u);
    return (ch579_lsr(id) & STA_TXALL_EMP) ? 1 : 0;
}

static void ch579_irq_callback_set(uart_t *u, uart_irq_callback cb, void *user_data)
{
    /* 回调由 uart.c 包装函数保存到 uart_t */
    (void)u; (void)cb; (void)user_data;
}

const uart_ops_t ch579_uart_ops = {
    .configure        = ch579_configure,
    .read             = ch579_read,
    .write            = ch579_write,
    .irq_rx_enable    = ch579_irq_rx_enable,
    .irq_rx_disable   = ch579_irq_rx_disable,
    .irq_rx_ready     = ch579_irq_rx_ready,
    .irq_tx_enable    = ch579_irq_tx_enable,
    .irq_tx_disable   = ch579_irq_tx_disable,
    .irq_tx_ready     = ch579_irq_tx_ready,
    .irq_tx_complete  = ch579_irq_tx_complete,
    .irq_callback_set = ch579_irq_callback_set,
};

/* ---- 中断总入口 ---- */

void ch579_uart_irq_handler(uint8_t id)
{
    uart_t *u = uart_get(id);
    if (!u)
        return;

    /* 读 IIR 清中断标志，即使没有回调也不会挂死 */
    switch (id) {
    case 0: (void)UART0_GetITFlag(); break;
    case 1: (void)UART1_GetITFlag(); break;
    case 2: (void)UART2_GetITFlag(); break;
    case 3: (void)UART3_GetITFlag(); break;
    default: break;
    }

    if (u->irq_cb)
        u->irq_cb(u, u->irq_user_data);
}

#endif /* __CH579__ */
