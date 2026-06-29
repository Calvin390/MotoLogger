//
// Created by cbumgardner on 6/29/2026.
//

#include <stdio.h>
#include <stdbool.h>

#include "esp_err.h"
#include "esp_log.h"

#include "driver/twai.h"

#include "config.h"
#include "CANbus.h"

static const char *TAG = "CAN_BUS";

static bool can_driver_started = false;

esp_err_t can_init(void)
{
    if (can_driver_started) {
        printf("CAN/TWAI is already started\n");
        return ESP_OK;
    }

    twai_general_config_t general_config = TWAI_GENERAL_CONFIG_DEFAULT(
        CAN_TX_GPIO,
        CAN_RX_GPIO,
        TWAI_MODE_NORMAL
    );

    twai_timing_config_t timing_config = TWAI_TIMING_CONFIG_500KBITS();
    twai_filter_config_t filter_config = TWAI_FILTER_CONFIG_ACCEPT_ALL();

    general_config.tx_queue_len = 10;
    general_config.rx_queue_len = 32;

    esp_err_t ret = twai_driver_install(
        &general_config,
        &timing_config,
        &filter_config
    );

    if (ret != ESP_OK) {
        printf("CAN driver install failed: %s\n", esp_err_to_name(ret));
        return ret;
    }

    ret = twai_start();

    if (ret != ESP_OK) {
        printf("CAN start failed: %s\n", esp_err_to_name(ret));
        twai_driver_uninstall();
        return ret;
    }

    can_driver_started = true;

    printf("CAN/TWAI started at 500 kbps\n");
    ESP_LOGI(TAG, "CAN/TWAI driver started");

    return ESP_OK;
}

esp_err_t can_stop(void)
{
    if (!can_driver_started) {
        printf("CAN/TWAI is not started\n");
        return ESP_OK;
    }

    esp_err_t ret = twai_stop();
    if (ret != ESP_OK) {
        printf("CAN stop failed: %s\n", esp_err_to_name(ret));
        return ret;
    }

    ret = twai_driver_uninstall();
    if (ret != ESP_OK) {
        printf("CAN driver uninstall failed: %s\n", esp_err_to_name(ret));
        return ret;
    }

    can_driver_started = false;

    printf("CAN/TWAI stopped\n");
    ESP_LOGI(TAG, "CAN/TWAI driver stopped");

    return ESP_OK;
}

bool is_can_started(void)
{
    return can_driver_started;
}