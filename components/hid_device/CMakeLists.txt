idf_component_register(
    SRCS "ble_hidd.c"
         "esp_hidd_prf_api.c"
         "hid_dev.c"
         "hid_device_le_prf.c"

    INCLUDE_DIRS "./"
                 "../shell"
                 "../shell/letter-shell"
                 "../shell/letter-shell/src"

    REQUIRES
        bt
        driver
)

target_compile_options(${COMPONENT_LIB} PRIVATE -Wno-unused-const-variable)


target_link_libraries(
    ${COMPONENT_LIB}
    
    INTERFACE
        "-u shellCommandhid_device"
        "-u shellCommandhid_send"
)
