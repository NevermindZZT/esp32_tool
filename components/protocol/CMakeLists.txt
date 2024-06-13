idf_component_register(
    SRCS
        "protocol_common.c"
        "serial_debug.c"
        "icon_app_serial_debug.c"
        "serial_debug_uart.c"
        "serial_debug_i2c.c"
        "serial_debug_spi.c"

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
        "-u rtAppserial_debug"
        "-u shellCommanduartd"
        "-u shellCommandi2cd"
        "-u shellCommandspid"
)

target_compile_options(${COMPONENT_LIB} PRIVATE -Wno-missing-braces)
