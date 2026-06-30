#include <stdio.h>
#include <stdbool.h>
#include <string.h>
#include <inttypes.h>

#include "esp_err.h"
#include "esp_log.h"
#include "esp_timer.h"

#include "driver/twai.h"

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/queue.h"

#include "config.h"
#include "CANbus.h"

static const char *TAG = "CAN_BUS";

static bool can_driver_started = false;

static TaskHandle_t can_rx_task_handle = NULL;
static QueueHandle_t can_log_queue = NULL;

static void can_message_to_log_record(const twai_message_t *message,
                                      can_log_record_t *record)
{
    if (message == NULL || record == NULL) {
        return;
    }

    record->time_us = esp_timer_get_time();
    record->id = message->identifier;
    record->dlc = message->data_length_code;
    record->extd = message->extd;
    record->rtr = message->rtr;

    memset(record->data, 0, sizeof(record->data));

    if (!message->rtr) {
        uint8_t copy_len = message->data_length_code;

        if (copy_len > 8) {
            copy_len = 8;
        }

        memcpy(record->data, message->data, copy_len);
    }
}

static void can_print_log_record(const can_log_record_t *record)
{
    if (record == NULL) {
        return;
    }

    if (record->extd) {
        printf("CAN RX EXT ID: 0x%08" PRIX32, record->id);
    } else {
        printf("CAN RX STD ID: 0x%03" PRIX32, record->id);
    }

    printf(" Time: %" PRId64 " us", record->time_us);
    printf(" DLC: %u", record->dlc);

    if (record->rtr) {
        printf(" RTR\n");
        return;
    }

    printf(" Data:");

    for (int i = 0; i < record->dlc && i < 8; i++) {
        printf(" %02X", record->data[i]);
    }

    printf("\n");
}

static void can_rx_task(void *arg)
{
    twai_message_t message;
    can_log_record_t record;

    while (1) {
        esp_err_t ret = twai_receive(&message, pdMS_TO_TICKS(1000));

        if (ret == ESP_OK) {
            can_message_to_log_record(&message, &record);

            if (can_log_queue != NULL) {
                if (xQueueSend(can_log_queue, &record, 0) != pdTRUE) {
                    ESP_LOGW(TAG, "CAN log queue full, dropping frame");
                }
            }

            can_print_log_record(&record);
        } else if (ret == ESP_ERR_TIMEOUT) {
            // Normal. No CAN frame received during this timeout.
        } else {
            ESP_LOGE(TAG, "twai_receive failed: %s", esp_err_to_name(ret));
            vTaskDelay(pdMS_TO_TICKS(100));
        }
    }
}

esp_err_t can_init(void)
{
    if (can_driver_started) {
        printf("CAN/TWAI is already started\n");
        return ESP_OK;
    }

    if (can_log_queue == NULL) {
        can_log_queue = xQueueCreate(CAN_LOG_QUEUE_LENGTH,
                                     sizeof(can_log_record_t));

        if (can_log_queue == NULL) {
            ESP_LOGE(TAG, "Failed to create CAN log queue");
            return ESP_ERR_NO_MEM;
        }
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

    general_config.alerts_enabled =
        TWAI_ALERT_RX_QUEUE_FULL |
        TWAI_ALERT_BUS_ERROR |
        TWAI_ALERT_BUS_OFF |
        TWAI_ALERT_ERR_PASS |
        TWAI_ALERT_ABOVE_ERR_WARN;

    esp_err_t ret = twai_driver_install(
        &general_config,
        &timing_config,
        &filter_config
    );

    if (ret != ESP_OK) {
        printf("CAN driver install failed: %s\n", esp_err_to_name(ret));

        if (can_log_queue != NULL) {
            vQueueDelete(can_log_queue);
            can_log_queue = NULL;
        }

        return ret;
    }

    ret = twai_start();

    if (ret != ESP_OK) {
        printf("CAN start failed: %s\n", esp_err_to_name(ret));
        twai_driver_uninstall();

        if (can_log_queue != NULL) {
            vQueueDelete(can_log_queue);
            can_log_queue = NULL;
        }

        return ret;
    }

    can_driver_started = true;

    printf("CAN/TWAI started at 500 kbps\n");
    ESP_LOGI(TAG, "CAN/TWAI driver started");

    return ESP_OK;
}

esp_err_t can_start_rx_task(void)
{
    if (!can_driver_started) {
        return ESP_ERR_INVALID_STATE;
    }

    if (can_rx_task_handle != NULL) {
        return ESP_OK;
    }

    if (can_log_queue == NULL) {
        can_log_queue = xQueueCreate(CAN_LOG_QUEUE_LENGTH,
                                     sizeof(can_log_record_t));

        if (can_log_queue == NULL) {
            ESP_LOGE(TAG, "Failed to create CAN log queue");
            return ESP_ERR_NO_MEM;
        }
    }

    BaseType_t task_ret = xTaskCreate(
        can_rx_task,
        "can_rx_task",
        4096,
        NULL,
        5,
        &can_rx_task_handle
    );

    if (task_ret != pdPASS) {
        ESP_LOGE(TAG, "Failed to create CAN RX task");
        can_rx_task_handle = NULL;
        return ESP_ERR_NO_MEM;
    }

    ESP_LOGI(TAG, "CAN RX task started");

    return ESP_OK;
}

esp_err_t can_stop_rx_task(void)
{
    if (can_rx_task_handle != NULL) {
        vTaskDelete(can_rx_task_handle);
        can_rx_task_handle = NULL;
        ESP_LOGI(TAG, "CAN RX task stopped");
    }

    return ESP_OK;
}

esp_err_t can_receive_log_record(can_log_record_t *record, TickType_t timeout)
{
    if (record == NULL) {
        return ESP_ERR_INVALID_ARG;
    }

    if (can_log_queue == NULL) {
        return ESP_ERR_INVALID_STATE;
    }

    if (xQueueReceive(can_log_queue, record, timeout) == pdTRUE) {
        return ESP_OK;
    }

    return ESP_ERR_TIMEOUT;
}

esp_err_t can_send_message(uint32_t id,
                           const uint8_t *data,
                           uint8_t length,
                           bool extended,
                           TickType_t timeout)
{
    if (!can_driver_started) {
        return ESP_ERR_INVALID_STATE;
    }

    if (length > 8) {
        return ESP_ERR_INVALID_ARG;
    }

    if (length > 0 && data == NULL) {
        return ESP_ERR_INVALID_ARG;
    }

    twai_message_t message = {0};

    message.identifier = id;
    message.extd = extended ? 1 : 0;
    message.rtr = 0;
    message.ss = 0;
    message.self = 0;
    message.dlc_non_comp = 0;
    message.data_length_code = length;

    if (length > 0) {
        memcpy(message.data, data, length);
    }

    return twai_transmit(&message, timeout);
}

esp_err_t can_stop(void)
{
    if (!can_driver_started) {
        printf("CAN/TWAI is not started\n");
        return ESP_OK;
    }

    can_stop_rx_task();

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

    if (can_log_queue != NULL) {
        vQueueDelete(can_log_queue);
        can_log_queue = NULL;
    }

    printf("CAN/TWAI stopped\n");
    ESP_LOGI(TAG, "CAN/TWAI driver stopped");

    return ESP_OK;
}

bool is_can_started(void)
{
    return can_driver_started;
}