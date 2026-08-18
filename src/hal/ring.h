#ifndef RING_H
#define RING_H
#include <stdint.h>

typedef struct {
    uint8_t *buf;
    uint16_t size;
    uint16_t mask;
    volatile uint16_t wridx;
    volatile uint16_t rdidx;
} ring_t;

void     ring_init(ring_t *r, uint8_t *buf, uint16_t size);
void     ring_reset(ring_t *r);
uint8_t  ring_empty(const ring_t *r);
uint8_t  ring_full(const ring_t *r);
uint16_t ring_count(const ring_t *r);
uint8_t  ring_put(ring_t *r, uint8_t byte);
uint16_t ring_write(ring_t *r, const uint8_t *src, uint16_t len);
uint8_t  ring_get(ring_t *r, uint8_t *out);
uint16_t ring_read(ring_t *r, uint8_t *dst, uint16_t max);
uint16_t ring_peek(ring_t *r, uint8_t *dst, uint16_t max);
/* 暂不使用，保留备用
int      ring_peek_at(const ring_t *r, uint16_t offset);
*/
void     ring_skip(ring_t *r, uint16_t n);
void     ring_commit(ring_t *r);
#endif
