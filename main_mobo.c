#include <stdio.h>
#include <string.h>
#include <stdbool.h>
#include <stdint.h>

#include "esp_system.h"
#include "esp_timer.h"
#include "esp_log.h"
#include "esp_err.h"

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freetos/queue.h"

#include "driver/twai.h"
#include "driver/gpio.h"
#include "driver/spi_common.h"
#include "driver/sdspi_host.h"

#include "sdmmc_cmd.h"
#include "esp_vfs_fat.h"

static const char *TAG = "MAIN";

#define MOUNT_POINT "/sdcard"

//GPIO PINS
#define PIN_NUM_MISO TBD
#define PIN_NUM_MOSI TBD
#define PIN_NUM_CLK  TBD
#define PIN_NUM_CS   TBD
#define CAN_TX_GPIO TBD
#define CAN_RX_GPIO TBD

//Global Defines
#define CAN_LOG_QUEUE_LENGTH 256

//Structs
typedef struct {
    int64_t time_us;
    uint32_t id;
    uint8_t dlc;
    bool extd;
    bool rtr;
} can_log_record_t;

//SD card globals
static const spi_host_device_t SD_SPI_HOST = SPI2_HOST;
static bool sd_card_mounted = false;
static sdmmc_card_t *sd_card = NULL;

//CANBUS Globals
static QueueHandle_t can_log_queue = NULL;
static FILE *log_file = NULL;
static bool logging_active = false;
static unsigned long dropped_log_frames = 0;
static bool can_driver_started = false;

// Function prototypes
static void command_options(void);
static void restart_esp32(void);
static void print_esp32_status(void);
static void ping(void);
static void command_handler(char *user_input);
static void usb_user_input(void *arg);

static esp_err_t init_sd_card(void);
static void rw_sd_card_test(void);
static void unmount_sd_card(void);


// The file is written with the intention that the ESP32-S3 will use USB Serial/JTAG
// USB output: printf(), ESP_LOGI()
// ESP_LOGI is timestamped + tagged output
//
// USB input: fgets()
//
// GPIO19/20 used for USB D-/D+

static void command_options(void)
{
    printf("Available options:\n");
    printf("1 - Restart ESP32\n");
    printf("2 - Print active ESP32 status\n");
    printf("3 - Ping USB response\n");
    printf("4 - mSD initialization\n");
    printf("5 - mSD read/write test\n");
    printf("6 - Unmount mSD card\n");
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
        init_sd_card();
    }
    else if (strcmp(user_input, "5") == 0) {
        rw_sd_card_test();
    }
    else if (strcmp(user_input, "6") == 0) {
        unmount_sd_card();
    }
    else if (strlen(user_input) == 0) {
        // Ignore blank input
    }
    else {
        printf("Invalid command: %s\n", user_input);
    }
}

static void usb_user_input(void *arg)
{
    char line[128];

    while (1) {
        printf("> ");
        fflush(stdout);

        if (fgets(line, sizeof(line), stdin) != NULL) {
            command_handler(line);
        }

        vTaskDelay(pdMS_TO_TICKS(10));
    }
}

static esp_err_t init_sd_card(void)
{
    if (sd_card_mounted) {
        printf("mSD card is already mounted\n");
        return ESP_OK;
    }

    esp_err_t ret;

    printf("Initializing SPI bus for mSD card...\n");

    sdmmc_host_t host = SDSPI_HOST_DEFAULT();
    host.slot = SD_SPI_HOST;

    spi_bus_config_t buscfg = {
        .mosi_io_num = PIN_NUM_MOSI,
        .miso_io_num = PIN_NUM_MISO,
        .sclk_io_num = PIN_NUM_CLK,
        .quadwp_io_num = -1,
        .quadhd_io_num = -1,
        .max_transfer_sz = 4000
    };

    ret = spi_bus_initialize(host.slot, &buscfg, SDSPI_DEFAULT_DMA);
    if (ret != ESP_OK) {
        printf("spi_bus_initialize failed: %s\n", esp_err_to_name(ret));
        return ret;
    }

    printf("Mounting mSD card...\n");

    sdspi_device_config_t slot_config = SDSPI_DEVICE_CONFIG_DEFAULT();
    slot_config.gpio_cs = PIN_NUM_CS;
    slot_config.host_id = host.slot;

    esp_vfs_fat_sdmmc_mount_config_t mount_config = {
        .format_if_mount_failed = false,
        .max_files = 5,
        .allocation_unit_size = 16 * 1024
    };

    ret = esp_vfs_fat_sdspi_mount(
        MOUNT_POINT,
        &host,
        &slot_config,
        &mount_config,
        &sd_card
    );

    if (ret != ESP_OK) {
        printf("Failed to mount SD card: %s\n", esp_err_to_name(ret));

        if (ret == ESP_FAIL) {
            printf("Filesystem mount failed\n");
        }
        else {
            printf("Card initialization failed\n");
        }

        spi_bus_free(host.slot);

        sd_card = NULL;
        sd_card_mounted = false;

        return ret;
    }

    sd_card_mounted = true;

    printf("mSD card mounted at %s\n", MOUNT_POINT);
    sdmmc_card_print_info(stdout, sd_card);

    return ESP_OK;
}

static void unmount_sd_card(void)
{
    if (!sd_card_mounted) {
        printf("mSD card is not mounted\n");
        return;
    }

    esp_vfs_fat_sdcard_unmount(MOUNT_POINT, sd_card);
    spi_bus_free(SD_SPI_HOST);

    sd_card = NULL;
    sd_card_mounted = false;

    printf("mSD card unmounted from %s\n", MOUNT_POINT);
}

static void rw_sd_card_test(void)
{
    if (!sd_card_mounted) {
        printf("mSD card is not mounted\n");
        return;
    }

    const char *file_path = MOUNT_POINT "/test.txt";

    printf("Writing test file: %s\n", file_path);

    FILE *file = fopen(file_path, "w");
    if (file == NULL) {
        printf("Failed to open file for writing\n");
        return;
    }

    fprintf(file, "ESP32-S3 microSD SPI test\n");
    fprintf(file, "Uptime: %lld ms\n", (long long)(esp_timer_get_time() / 1000));

    fclose(file);

    printf("File written successfully\n");

    printf("Reading test file: %s\n", file_path);

    file = fopen(file_path, "r");
    if (file == NULL) {
        printf("Failed to open file for reading\n");
        return;
    }

    char line[128];

    while (fgets(line, sizeof(line), file) != NULL) {
        printf("%s", line);
    }

    fclose(file);

    printf("\nmSD card test successful\n");
}

static esp_err_t can_init(void) {
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

    printf("CAN/TWAI started at 500kbps\n");

    return ESP_OK;
}

void app_main(void)
{
    setvbuf(stdout, NULL, _IONBF, 0);

    ESP_LOGI(TAG, "Starting ESP32");

    printf("USB Serial/JTAG console is active\n");
    command_options();

    xTaskCreate(
        usb_user_input,
        "usb_user_input",
        4096,
        NULL,
        5,
        NULL
    );
}