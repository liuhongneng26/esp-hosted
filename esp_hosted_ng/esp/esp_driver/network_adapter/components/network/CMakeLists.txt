set(src_dirs
            .
            )

set(include_dirs
            .
            )

idf_component_register(
    SRC_DIRS ${src_dirs}
    INCLUDE_DIRS ${include_dirs}
    PRIV_REQUIRES esp_wifi nvs_flash mqtt wifi_provisioning espressif__qrcode console espressif__iperf-cmd
    )