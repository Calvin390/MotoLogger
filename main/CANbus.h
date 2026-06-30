//
// Created by cbumgardner on 6/29/2026.
//

#ifndef ESP32_CAN_H
#define ESP32_CAN_H

#include <stdbool.h>
#include <stdint.h>

#include "esp_err.h"
#include "freertos/FreeRTOS.h"

typedef struct {
    int64_t time_us;
    uint32_t id;
    uint8_t dlc;
    bool extd;
    bool rtr;
    uint8_t data[8];
} can_log_record_t;

esp_err_t can_init(void);
esp_err_t can_stop(void);
bool is_can_started(void);

esp_err_t can_start_rx_task(void);
esp_err_t can_stop_rx_task(void);

esp_err_t can_receive_log_record(can_log_record_t *record, TickType_t timeout);

esp_err_t can_send_message(uint32_t id,
                           const uint8_t *data,
                           uint8_t length,
                           bool extended,
                           TickType_t timeout);

#endif //ESP32_CAN_H
