#include <stdio.h>

#include "esp_log.h"
#include "esp_err.h"

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

#include "console.h"
#include "CANbus.h"

static const char *TAG = "MAIN";

void app_main(void)
{
    // Disable stdout buffering so USB serial prints immediately
    setvbuf(stdout, NULL, _IONBF, 0);

    ESP_LOGI(TAG, "Starting ESP32-S3 MotoLogger");
    printf("USB Serial/JTAG console is active\n");

    ESP_ERROR_CHECK(can_init());
    ESP_ERROR_CHECK(can_start_rx_task());

    esp_err_t ret = console_start();
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "Failed to start console: %s", esp_err_to_name(ret));
        return;
    }

    ESP_LOGI(TAG, "Console started successfully");

    while (1) {
        vTaskDelay(pdMS_TO_TICKS(1000));
    }
}