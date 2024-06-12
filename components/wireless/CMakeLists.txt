idf_component_register(
    SRCS
        "wifi_service.c"
        "smartconfig.c"
        "icon_app_smartconfig.c"

    INCLUDE_DIRS "./"

    REQUIRES
        driver
        sys
        rtam
        lvgl
        launcher
        gui
)

target_link_libraries(
    ${COMPONENT_LIB}
    
    INTERFACE
        "-u rtAppsmartconfig"
        "-u rtAppwifi_service"
)
