#ifndef BUS_H
#define BUS_H
#include <stdint.h>

typedef struct {
    volatile uint8_t  busy;
    uint16_t          gap_ms;
    volatile uint32_t gap_until;
} bus_t;

void bus_init(bus_t *la, uint32_t baudrate);
void bus_mark_busy(bus_t *la);
void bus_mark_idle(bus_t *la);
int  bus_is_idle(const bus_t *la);
#endif
