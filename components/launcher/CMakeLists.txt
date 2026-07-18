idf_component_register(
    SRCS "launcher.c"

    INCLUDE_DIRS "./"
                 "../shell"
                 "../shell/letter-shell"
                 "../shell/letter-shell/src"
                 "../lvgl"
                 "../gui"
                 "../rtam"
                 "../rtam/rtam/src"

    REQUIRES
        esp_driver_gpio
        lvgl
        rtam
        gui
        cpost
)

target_link_libraries(
    ${COMPONENT_LIB} 

    INTERFACE 
        "-u rtApplauncher"
        "-u shellCommandhome"
)
