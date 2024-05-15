idf_component_register(
    SRCS "setting.c"
         "setting_common.c"
         "setting_mechanice.c"
         "setting_about.c"
         "icon_app_setting.c"
         "icon_app_setting_mechanice.c"
         "icon_app_setting_about.c"
         "icon_app_setting_brightness.c"

    INCLUDE_DIRS "./"
                 "../shell"
                 "../shell/letter-shell"
                 "../shell/letter-shell/src"
                 "../lvgl"
                 "../lvgl_esp32_drivers"
                 "../launcher"
                 "../gui"
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
        gui
)

target_compile_options(${COMPONENT_LIB} PRIVATE -DPROJECT_VER="${PROJECT_VER}")