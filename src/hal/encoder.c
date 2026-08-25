#include "encoder.h"

uint8_t encoder_configure(encoder_t *e, const void *cfg)
{
    if (!e || !e->ops || !e->ops->configure) return 1;
    return e->ops->configure(e, cfg);
}

uint8_t encoder_encode_byte(encoder_t *e, uint8_t byte)
{
    if (!e || !e->ops || !e->ops->encode_byte) return 1;
    return e->ops->encode_byte(e, byte);
}

void encoder_tx_enable(encoder_t *e)
{
    if (e && e->ops && e->ops->tx_enable)
        e->ops->tx_enable(e);
}

void encoder_tx_disable(encoder_t *e)
{
    if (e && e->ops && e->ops->tx_disable)
        e->ops->tx_disable(e);
}

uint8_t encoder_tx_ready(encoder_t *e)
{
    if (!e || !e->ops || !e->ops->tx_ready) return 0;
    return e->ops->tx_ready(e);
}

uint8_t encoder_tx_complete(encoder_t *e)
{
    if (!e || !e->ops || !e->ops->tx_complete) return 0;
    return e->ops->tx_complete(e);
}
