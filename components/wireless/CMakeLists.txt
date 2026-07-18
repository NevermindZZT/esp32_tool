idf_component_register(
    SRCS
        "wifi_service.c"
        "smartconfig.c"
        "icon_app_smartconfig.c"

    INCLUDE_DIRS "./"

    REQUIRES
        esp_driver_gpio
        sys
        rtam
        lvgl
        launcher
        gui
        shell
)

target_link_libraries(
    ${COMPONENT_LIB}
    
    INTERFACE
        "-u rtAppsmartconfig"
        "-u rtAppwifi_service"
        "-u shellCommandwifi"
)
