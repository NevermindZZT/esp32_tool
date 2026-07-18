idf_component_register(
    SRCS
        "multimeter.c"
        "icon_app_multimeter.c"
        "ina226.c"

    INCLUDE_DIRS
        "./"

    REQUIRES
        esp_driver_gpio
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

