#ifndef PHY_UART1_H
#define PHY_UART1_H
#include "phy.h"

typedef struct sender sender_t;

phy_driver_t *phy_uart1_create(sender_t *sender);
void phy_uart1_set_sender(phy_driver_t *phy, sender_t *sender);
#endif
