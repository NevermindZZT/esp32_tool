idf_component_register(
    SRCS "icon_app_test.c" "test.c"

    INCLUDE_DIRS "./"
                 "../shell"
                 "../shell/letter-shell"
                 "../shell/letter-shell/src"
                 "../lvgl"
                 "../lvgl_esp32_drivers"
                 "../launcher"
                 "../rtam"
                 "../rtam/rtam/src"

    REQUIRES
        driver
        spi_flash
        esp_hw_support
        lvgl
        rtam
        lvgl_esp32_drivers
        launcher
)

target_link_libraries(${COMPONENT_LIB} INTERFACE "-u rtApptest")
