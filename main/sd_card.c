//
// Created by cbumgardner on 6/29/2026.
//
#include <stdio.h>
#include <stdbool.h>

#include "esp_err.h"
#include "esp_log.h"
#include "esp_timer.h"

#include "driver/spi_common.h"
#include "driver/sdspi_host.h"

#include "sdmmc_cmd.h"
#include "esp_vfs_fat.h"

#include "config.h"
#include "sd_card.h"

static const char *TAG = "SD_CARD";

// SD card globals
static const spi_host_device_t SD_SPI_HOST = SPI2_HOST;
static bool sd_card_mounted = false;
static sdmmc_card_t *sd_card = NULL;

esp_err_t init_sd_card(void)
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

void unmount_sd_card(void)
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

void rw_sd_card_test(void)
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

bool is_sd_card_mounted(void)
{
    return sd_card_mounted;
}