cmake_minimum_required(VERSION 4.2)
project(ESP32 C)

set(CMAKE_C_STANDARD 23)

add_executable(ESP32 main_mobo.c
        config.h
        config.h
        config.h
        sd_card.c
        sd_card.h
        console.c
        console.h
        CANbus.c
        CANbus.h)
