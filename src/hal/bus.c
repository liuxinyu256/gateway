#include "bus.h"
#ifdef FAKE_FREERTOS
#include "fake_freertos.h"
#else
#include "FreeRTOS.h"
#endif
#include <string.h>

void bus_init(bus_t *la, uint32_t baudrate) {
    memset(la, 0, sizeof(*la));
    la->gap_ms = (uint16_t)(35000UL / baudrate);
    if (la->gap_ms < 1)  la->gap_ms = 1;
    if (la->gap_ms > 10) la->gap_ms = 10;
}

void bus_mark_busy(bus_t *la) {
    la->busy = 1;
}

void bus_mark_idle(bus_t *la) {
    la->busy = 0;
    la->gap_until = xTaskGetTickCount() + pdMS_TO_TICKS(la->gap_ms);
}

int bus_is_idle(const bus_t *la) {
    return !la->busy && xTaskGetTickCount() >= la->gap_until;
}
