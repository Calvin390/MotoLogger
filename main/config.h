//
// Created by cbumgardner on 6/29/2026.
//

#ifndef MOTO_MOBO_H
#define MOTO_MOBO_H

#define MOUNT_POINT "/sdcard"
#include "driver/gpio.h"

// GPIO pins
#define PIN_NUM_MISO  GPIO_NUM_13
#define PIN_NUM_MOSI  GPIO_NUM_11
#define PIN_NUM_CLK   GPIO_NUM_12
#define PIN_NUM_CS    GPIO_NUM_10

#define CAN_TX_GPIO   GPIO_NUM_21
#define CAN_RX_GPIO   GPIO_NUM_38

#define CAN_LOG_QUEUE_LENGTH 256

#endif //MOTO_MOBO_H
