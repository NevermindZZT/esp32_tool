idf_component_register(
    SRCS
        "multimeter.c"
        "icon_app_multimeter.c"
        "ina226.c"

    INCLUDE_DIRS
        "./"

    REQUIRES
        driver
        rtam
        shell
        lvgl
        gui
        launcher
        cpost
)

target_link_libraries(
    ${COMPONENT_LIB}
    
    INTERFACE
        "-u rtAppmultimeter"
        "-u shellCommandmultimeter"
)

