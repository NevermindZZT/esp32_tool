idf_component_register(
    SRCS "ble_remote.c"
         "esp_hidd_prf_api.c"
         "hid_dev.c"
         "hid_device_le_prf.c"
         "icon_app_ble_remote.c"

    INCLUDE_DIRS "./"

    REQUIRES
        bt
        bt_manager
        esp_driver_gpio
        gui
        launcher
        rtam
        shell
)

target_compile_options(${COMPONENT_LIB} PRIVATE
    -Wno-unused-const-variable
    -Wno-incompatible-pointer-types
    -Wno-cast-function-type)

target_link_libraries(
    ${COMPONENT_LIB}

    INTERFACE
        "-u rtAppble_remote"
        "-u shellCommandble_remote"
)
