# Motologger

**Motologger** is a modular motorcycle data logger built around an ESP32-S3.  
The goal of the project is to record useful riding and vehicle dynamics data from a motorcycle that does not have a factory ECU or CAN bus.

The system is designed to collect data from external sensor modules over CAN and log it to a microSD card for later analysis.

> Current target platform: 2001 Suzuki SV650  
> Current firmware target: ESP32-S3 using ESP-IDF in C

---

## Project Goals

Motologger is intended to be a compact, low-cost motorcycle telemetry system capable of logging:

- IMU data such as acceleration, gyro, and estimated lean angle
- GPS position and speed
- CAN messages from external daughterboards
- Timestamped CSV data to a microSD card
- Future expansion for additional sensors such as throttle position, brake pressure, wheel speed, and steering angle

The long-term goal is to build a modular logging and display system that can support both post-ride analysis and a future visor-mounted HUD.

---

## Current Status

This project is currently in the prototype/firmware development stage.

Current firmware focus:

- ESP32-S3 bring-up
- microSD card logging over SPI
- CAN receive/transmit structure
- Sensor packet format
- CSV logging format
- Basic serial debug interface

Hardware is also under active development and may change between revisions.

---

## System Overview

The Motologger system is split into a main controller board and external sensor modules.

```text
       +--------------------+
       |    GPS Module       |
       |   u-blox M10        |
       +----------+---------+
                  |
                  | CAN
                  |
+-----------------v-----------------+
|          Motologger Mainboard      |
|                                    |
|  ESP32-S3                          |
|  microSD Card                      |
|  CAN Transceiver                   |
|  USB-C Debug/Programming           |
|  12V Motorcycle Power Input        |
+-----------------^-----------------+
                  |
                  | CAN
                  |
       +----------+---------+
       |     IMU Module      |
       |   ICM-20948         |
       +--------------------+
