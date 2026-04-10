set(src_dirs
            .
            gpio
            i2c
            spi
            )

set(include_dirs
            .
            ./gpio
            ./i2c
            ./spi
            )

idf_component_register(
    SRC_DIRS ${src_dirs}
    INCLUDE_DIRS ${include_dirs}
    REQUIRES pthread esp_lcd main esp_hal_i2c
    )

set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -Wno-missing-field-initializers -Wno-type-limits")