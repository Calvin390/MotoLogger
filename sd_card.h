//
// Created by cbumgardner on 6/29/2026.
//

#ifndef ESP32_SD_CARD_H
#define ESP32_SD_CARD_H

#include <stdbool.h>
#include "esp_err.h"

esp_err_t init_sd_card(void);
void rw_sd_card_test(void);
void unmount_sd_card(void);
bool is_sd_card_mounted(void);

#endif //ESP32_SD_CARD_H
