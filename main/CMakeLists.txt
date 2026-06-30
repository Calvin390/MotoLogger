idf_component_register(
        SRCS
        "main_mobo.c"
        "console.c"
        "sd_card.c"
        "CANbus.c"
        "can_protocol.c"

        INCLUDE_DIRS
        "."

        REQUIRES
        driver
        esp_timer
        freertos
        fatfs
        sdmmc
)
