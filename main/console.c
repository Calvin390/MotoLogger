//
// Created by cbumgardner on 6/29/2026.
//
#include <stdio.h>
#include <string.h>
#include <stdint.h>

#include "esp_err.h"
#include "esp_system.h"
#include "esp_timer.h"
#include "esp_log.h"

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

#include "console.h"
#include "sd_card.h"
#include "CANbus.h"

static const char *TAG = "CONSOLE";

static void command_options(void);
static void restart_esp32(void);
static void print_esp32_status(void);
static void ping(void);
static void command_handler(char *user_input);
static void usb_user_input(void *arg);

static void command_options(void)
{
    printf("\n");
    printf("Available options:\n");
    printf("1 - Restart ESP32\n");
    printf("2 - Print active ESP32 status\n");
    printf("3 - Ping USB response\n");
    printf("4 - mSD initialization\n");
    printf("5 - mSD read/write test\n");
    printf("6 - Unmount mSD card\n");
    printf("7 - Start CAN/TWAI\n");
    printf("h - Print this help menu\n");
    printf("\n");
}

static void restart_esp32(void)
{
    printf("Restarting ESP32 in 3 seconds...\n");
    fflush(stdout);

    vTaskDelay(pdMS_TO_TICKS(3000));

    esp_restart();
}

static void print_esp32_status(void)
{
    esp_chip_info_t chip_info;
    esp_chip_info(&chip_info);

    unsigned int free_heap = esp_get_free_heap_size();
    int64_t uptime_ms = esp_timer_get_time() / 1000;

    printf("\n");
    printf("Active ESP32 status:\n");
    printf("Core Count: %d\n", chip_info.cores);
    printf("Uptime: %lld ms\n", (long long)uptime_ms);
    printf("Free Heap Size: %u bytes\n", free_heap);
    printf("\n");
}

static void ping(void)
{
    printf("USB Active: Pong\n");
}

static void command_handler(char *user_input)
{
    user_input[strcspn(user_input, "\r\n")] = '\0';

    if (strcmp(user_input, "1") == 0) {
        restart_esp32();
    }
    else if (strcmp(user_input, "2") == 0) {
        print_esp32_status();
    }
    else if (strcmp(user_input, "3") == 0) {
        ping();
    }
    else if (strcmp(user_input, "4") == 0) {
        esp_err_t ret = init_sd_card();
        if (ret != ESP_OK) {
            printf("mSD initialization failed: %s\n", esp_err_to_name(ret));
        }
    }
    else if (strcmp(user_input, "5") == 0) {
        rw_sd_card_test();
    }
    else if (strcmp(user_input, "6") == 0) {
        unmount_sd_card();
    }
    else if (strcmp(user_input, "7") == 0) {
        esp_err_t ret = can_init();
        if (ret != ESP_OK) {
            printf("CAN/TWAI start failed: %s\n", esp_err_to_name(ret));
        }
    }
    else if ((strcmp(user_input, "h") == 0) || (strcmp(user_input, "help") == 0)) {
        command_options();
    }
    else if (strlen(user_input) == 0) {
        // Ignore blank input
    }
    else {
        printf("Invalid command: %s\n", user_input);
        printf("Type h for help\n");
    }
}

static void usb_user_input(void *arg)
{
    char line[128];

    command_options();

    while (1) {
        printf("> ");
        fflush(stdout);

        if (fgets(line, sizeof(line), stdin) != NULL) {
            command_handler(line);
        }

        vTaskDelay(pdMS_TO_TICKS(10));
    }
}

esp_err_t console_start(void)
{
    BaseType_t result = xTaskCreate(
        usb_user_input,
        "usb_user_input",
        4096,
        NULL,
        5,
        NULL
    );

    if (result != pdPASS) {
        ESP_LOGE(TAG, "Failed to create USB console task");
        return ESP_FAIL;
    }

    ESP_LOGI(TAG, "USB console task started");

    return ESP_OK;
}