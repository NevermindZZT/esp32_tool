idf_component_register(
    SRCS
        "screensaver.c"

    INCLUDE_DIRS "./"

    REQUIRES
        driver
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
        "-u rtAppscreensaver"
)
