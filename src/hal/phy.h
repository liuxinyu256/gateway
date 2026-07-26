#ifndef PHY_H
#define PHY_H
#include <stdint.h>

typedef struct phy_driver phy_driver_t;

struct phy_driver {
    int  (*open)(phy_driver_t *self);
    void (*close)(phy_driver_t *self);
    void (*write)(phy_driver_t *self, uint8_t byte);
    void (*forward_received_byte)(phy_driver_t *self, uint8_t byte);
    void (*set_receive_callback)(phy_driver_t *self, void (*cb)(uint8_t byte, void *ctx), void *ctx);
    uint8_t sending;
    uint8_t half_duplex;
};

typedef enum {
    PHY_RS485_8N1 = 0x10,  PHY_RS485_8E1 = 0x11,
    PHY_TTL_8N1   = 0x20,  PHY_TTL_8E1   = 0x21,  PHY_TTL_8N2 = 0x23,
    PHY_HBS       = 0x30,
    PHY_SW_UART   = 0x40,
} phy_type_t;

typedef struct {
    phy_type_t  type;
    uint8_t     uart_id;
    uint32_t    baudrate;
    uint8_t     tx_pin, rx_pin, de_pin;
} phy_config_t;

phy_driver_t *phy_create(const phy_config_t *cfg);
#endif

