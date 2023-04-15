idf_component_register(
    SRCS "ble_hidd.c"
         "esp_hidd_prf_api.c"
         "hid_dev.c"
         "hid_device_le_prf.c"

    INCLUDE_DIRS "./"
                 "../shell"
                 "../shell/src"

    REQUIRES
        bt
        driver
)

target_compile_options(${COMPONENT_LIB} PRIVATE -Wno-unused-const-variable)
