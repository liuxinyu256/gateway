#ifndef MODULE_H
#define MODULE_H
#include <stdint.h>
#include "phy.h"
#include "receiver.h"
#include "bus.h"
#include "sender.h"
#include "event_handler.h"
#ifdef FAKE_FREERTOS
#include "fake_freertos.h"
#else
#include "FreeRTOS.h"
#include "task.h"
#include "queue.h"
#include "timers.h"
#endif

#define RING_HVAC_AC     128    /* 空调 Modbus */
#define RING_THIRD_PARTY 128    /* 第三方485 */
#define RING_WIRELESS     64    /* 无线模组/WiFi */
#define RX_BUF_SIZE      128

typedef struct module {
    uint8_t              id;
    phy_driver_t        *phy;
    bus_t       bus;
    sender_t             sender;
    TaskHandle_t         rx_task;
    TaskHandle_t         send_task;
    QueueHandle_t        send_queue;
    TimerHandle_t        poll_timer;
    TimerHandle_t        timeout_timer;
    receiver_t        *pkt;
    const event_handler_t *handler;
    void                 *handler_ctx;

    uint8_t              rx_ring_buf[128];
    uint8_t              tx_ring_buf[128];
    uint16_t             ring_size;
} module_t;

int  module_init(module_t *m, uint8_t id, phy_driver_t *phy,
                  uint32_t baudrate, uint16_t ring_size,
                  frame_timer_t *timer, uint16_t timeout_ticks);
void module_start(module_t *m);
int  module_send_cmd(module_t *m, uint8_t cmd, uint8_t val);
void module_set_poll_period(module_t *m, uint16_t period_ms);
#endif
