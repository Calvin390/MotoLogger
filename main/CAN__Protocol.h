//
// Created by cbumgardner on 6/30/2026.
//

#ifndef ESP32_CAN__PROTOCOL_H
#define ESP32_CAN__PROTOCOL_H

#include <stdint.h>

#define CAN_ID_IMU_ACCEL 0x100
#define CAN_ID_IMU_GYRO 0x101
#define CAN_ID_IMU_ATTITUDE 0x102
#define CAN_ID_IMU_MAG_TEMP 0x103

#define CAN_ID_GPS_FIX_STATUS 0x200
#define CAN_ID_GPS_LAT_LON 0x201
#define CAN_ID_GPS_ALT_SPEED_HEADING 0x203

#define CAN_ID_NODE_HEARTBEAT_BASE 0x300
#define CAN_ID_ERROR_STATUS 0x301

typedef enum {
    NODE_ID_LOGGER = 1,
    NODE_ID_IMU    = 2,
    NODE_ID_GPS    = 3
} can_node_id_t;

typedef enum {
    GPS_FIX_NONE = 0,
    GPS_FIX_DR_ONLY = 1,
    GPS_FIX_2D = 2,
    GPS_FIX_3D = 3,
    GPS_FIX_GNSS_DR = 4,
    GPS_FIX_RTK_FLOAT = 5,
    GPS_FIX_RTK_FIXED = 6
} gps_fix_type_t;


#endif //ESP32_CAN__PROTOCOL_H
