//
// Created by cbumgardner on 6/30/2026.
//
#include <stdint.h>

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

#include "CANbus.h"
#include "can_protocol.h"

static void put_u16_le(uint8_t *buf, uint16_t value) {
    buf[0] = (uint8_t)(value & 0xFF);
    buf[1] = (uint8_t)((value >> 8) & 0xFF);
}

static void put_i16_le(uint8_t *buf, int16_t value) {
    put_u16_le(buf, (uint16_t)value);
}

static void put_u32_le(uint8_t *buf, uint32_t value) {
    buf[0] = (uint8_t)(value & 0xFF);
    buf[1] = (uint8_t)((value >> 8) & 0xFF);
    buf[2] = (uint8_t)((value >> 16) & 0xFF);
    buf[3] = (uint8_t)((value >> 24) & 0xFF);
}
static void put_i32_le(uint8_t *buf, int32_t value) {
    put_u32_le(buf, (uint32_t)value);
}
esp_err_t can_protocol_send_imu_accel(int16_t ax_mg, int16_t ay_mg, int16_t az_mg, uint8_t sample_counter, uint8_t status_flags) {
    uint8_t data[8] = {0};

    put_i16_le(&data[0], ax_mg);
    put_i16_le(&data[2], ay_mg);
    put_i16_le(&data[4], az_mg);
    data[6] = sample_counter;
    data[7] = status_flags;

    return can_send_message(CAN_ID_IMU_ACCEL, data, 8, false, pdMS_TO_TICKS(10));
}

esp_err_t can_protocol_send_gps_lat_lon(int32_t latitude_e7,
                                        int32_t longitude_e7)
{
    uint8_t data[8] = {0};

    put_i32_le(&data[0], latitude_e7);
    put_i32_le(&data[4], longitude_e7);

    return can_send_message(CAN_ID_GPS_LAT_LON,
                            data,
                            8,
                            false,
                            pdMS_TO_TICKS(10));
}