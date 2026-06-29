//
// Created by cbumgardner on 6/29/2026.
//

#ifndef ESP32_CAN_H
#define ESP32_CAN_H

#include <stdbool.h>
#include <stdint.h>

#include "esp_err.h"

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

#endif //ESP32_CAN_H
