#ifndef BRAND_H
#define BRAND_H
#include <stdint.h>

typedef enum {
    BRAND_ID_GREE           = 0x01,
    BRAND_ID_MIDEA          = 0x02,
    BRAND_ID_LANSHE         = 0x03,
    BRAND_ID_MIJIA_WIFI     = 0x10,
    BRAND_ID_MIJIA_BLE      = 0x11,
    BRAND_ID_TUYA           = 0x12,
    BRAND_ID_ESPRESSIF      = 0x20,
    BRAND_ID_REALTEK        = 0x21,
    BRAND_ID_CUSTOM         = 0x80,
} brand_id_t;

typedef enum { DEV_AC, DEV_FRESH_AIR, DEV_FLOOR_HEAT } device_type_t;

typedef enum {
    B_IDLE, B_WAIT_QUERY_RESP, B_WAIT_CTRL_ACK,
    B_WAIT_HANDSHAKE, B_BRAND_CUSTOM = 0x10,
} brand_state_t;

typedef struct module module_t;

typedef struct brand {
    module_t       *mod;
    brand_id_t      brand_id;
    device_type_t   dev_type;
    brand_state_t   state;
    uint32_t        timeout_at;
    uint8_t         retry;
    uint8_t         max_retry;
    uint8_t         poll_phase;
} brand_t;

#endif
