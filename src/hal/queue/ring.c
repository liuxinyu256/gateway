#include "ring.h"

void ring_init(ring_t *r, uint8_t *buf, uint16_t sz)
{
    if (!r || !buf || !sz) return;
    r->buf  = buf;
    r->size = sz;
    r->mask = sz - 1;
    r->wridx = r->rdidx = 0;
}

void ring_reset(ring_t *r)
{
    if (r) r->rdidx = r->wridx;
}

uint8_t ring_empty(const ring_t *r)
{
    return !r || r->rdidx == r->wridx;
}

uint8_t ring_full(const ring_t *r)
{
    return !r || ((r->wridx + 1) & r->mask) == r->rdidx;
}

uint16_t ring_count(const ring_t *r)
{
    return !r ? 0 : (r->wridx - r->rdidx) & r->mask;
}

uint8_t ring_put(ring_t *r, uint8_t b)
{
    if (!r || ring_full(r)) return 1;
    r->buf[r->wridx] = b;
    r->wridx = (r->wridx + 1) & r->mask;
    return 0;
}

uint16_t ring_write(ring_t *r, const uint8_t *s, uint16_t l)
{
    if (!r || !s || !l) return 0;
    uint16_t w = 0;
    for (uint16_t i = 0; i < l; i++) {
        if (ring_put(r, s[i])) break;
        w++;
    }
    return w;
}

uint8_t ring_get(ring_t *r, uint8_t *o)
{
    if (!r || !o || ring_empty(r)) return 1;
    *o = r->buf[r->rdidx];
    r->rdidx = (r->rdidx + 1) & r->mask;
    return 0;
}

static uint16_t _cp(const ring_t *r, uint8_t *d, uint16_t m)
{
    if (!r || !d || !m) return 0;
    uint16_t n = ring_count(r);
    if (n > m) n = m;
    if (!n) return 0;
    uint16_t s = r->rdidx;
    if (s + n <= r->size) {
        for (uint16_t i = 0; i < n; i++) d[i] = r->buf[s + i];
    } else {
        uint16_t f = r->size - s;
        for (uint16_t i = 0; i < f; i++) d[i] = r->buf[s + i];
        for (uint16_t i = 0; i < n - f; i++) d[f + i] = r->buf[i];
    }
    return n;
}

uint16_t ring_read(ring_t *r, uint8_t *d, uint16_t m)
{
    uint16_t n = _cp(r, d, m);
    if (r) r->rdidx = (r->rdidx + n) & r->mask;
    return n;
}

uint16_t ring_peek(ring_t *r, uint8_t *d, uint16_t m)
{
    return _cp(r, d, m);
}

/* 暂不使用，保留备用
int ring_peek_at(const ring_t *r, uint16_t o)
{
    if (!r || o >= ring_count(r))
        return -1;
    return r->buf[(r->rdidx + o) & r->mask];
}
*/

void ring_skip(ring_t *r, uint16_t n)
{
    if (!r) return;
    uint16_t cnt = ring_count(r);
    if (n > cnt) n = cnt; /* 不允许跳过超过实际数据量 */
    r->rdidx = (r->rdidx + n) & r->mask;
}

void ring_commit(ring_t *r)
{
    if (r) r->rdidx = r->wridx;
}
