#ifndef GATEWAY_DEVICE_H
#define GATEWAY_DEVICE_H
#include <stdint.h>
#ifdef FAKE_FREERTOS
#include "fake_freertos.h"
#else
#include "FreeRTOS.h"
#include "semphr.h"
#endif

typedef struct module module_t;

typedef struct {
    uint8_t power, mode, set_temp, room_temp;
    uint8_t fan, swing, error_code;
} gateway_state_t;

typedef void (*state_change_cb)(const gateway_state_t *s, void *ctx);

typedef struct gateway_device {
    gateway_state_t    state;
    SemaphoreHandle_t  state_mutex;

    module_t          *modules[5];

    state_change_cb    on_change[8];
    void              *on_change_ctx[8];
    uint8_t            observer_count;
} gateway_device_t;

void gateway_init(void);
int  gateway_send_cmd(uint8_t module_id, uint8_t cmd, uint8_t val);
void gateway_state_update(const gateway_state_t *s);
void gateway_state_get(gateway_state_t *out);
void gateway_on_state_change(state_change_cb cb, void *ctx);
module_t *gateway_module(uint8_t id);
void      gateway_set_module(uint8_t id, module_t *m);
#endif
