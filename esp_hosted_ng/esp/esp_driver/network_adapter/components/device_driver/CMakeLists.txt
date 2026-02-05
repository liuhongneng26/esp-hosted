set(src_dirs
            .
            input
            sensor
            display/ssd1306
            )

set(include_dirs
            .
            input
            sensor
            display/ssd1306
            )

idf_component_register(
    SRC_DIRS ${src_dirs}
    INCLUDE_DIRS ${include_dirs}
    PRIV_REQUIRES driver pthread esp_timer peripheral espressif__button esp_lcd esp_driver_usb_serial_jtag
    )

set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -Wno-missing-field-initializers")