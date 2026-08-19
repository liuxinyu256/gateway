#include "decoder.h"

int decoder_init(decoder_t *d, const void *cfg)
{
    if (!d || !d->ops || !d->ops->init)
        return -1;
    return d->ops->init(d, cfg);
}

void decoder_set_rx_callback(decoder_t *d, decoder_rx_cb cb, void *ctx)
{
    if (!d) return;

    d->rx_cb  = cb;
    d->rx_ctx = ctx;

    if (d->ops && d->ops->set_rx_callback)
        d->ops->set_rx_callback(d, cb, ctx);
}

void decoder_feed_byte(decoder_t *d, uint8_t byte)
{
    if (!d || !d->ops || !d->ops->feed_byte)
        return;
    d->ops->feed_byte(d, byte);
}

void decoder_feed_sample(decoder_t *d, uint8_t level, uint32_t ts_us)
{
    if (!d || !d->ops || !d->ops->feed_sample)
        return;
    d->ops->feed_sample(d, level, ts_us);
}
