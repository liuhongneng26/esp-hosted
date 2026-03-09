set(src_dirs
            .
            )

set(include_dirs
            .
            )

idf_component_register(
    SRC_DIRS ${src_dirs}
    INCLUDE_DIRS ${include_dirs}
    REQUIRES console PRIV_REQUIRES console espressif__iperf-cmd esp_driver_usb_serial_jtag espressif__iperf-cmd espressif__console_cmd_ping
    peripheral
    )