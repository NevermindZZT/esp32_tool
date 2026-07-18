idf_component_register(
    SRCS
        "screensaver.c"
        "simple_time.c"
        "mech_watch.c"

    INCLUDE_DIRS "./"

    REQUIRES
        esp_driver_gpio
        sys
        rtam
        lvgl
        launcher
        gui
        shell
        key
        setting
)

target_link_libraries(
    ${COMPONENT_LIB}
    
    INTERFACE
        "-u rtAppscreensaver"
)
