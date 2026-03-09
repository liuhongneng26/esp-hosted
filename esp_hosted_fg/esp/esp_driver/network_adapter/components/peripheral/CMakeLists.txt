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
    PRIV_REQUIRES driver pthread esp_lcd main
    )

set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -Wno-missing-field-initializers -Wno-type-limits")