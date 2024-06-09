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

    REQUIRES
        driver
        spi_flash
        esp_hw_support
        lvgl
        rtam
        lvgl_esp32_drivers
        launcher
        gui
        battery
)

target_compile_options(${COMPONENT_LIB} PRIVATE -DPROJECT_VER="${PROJECT_VER}")
target_link_libraries(${COMPONENT_LIB} INTERFACE "-u rtAppsetting")
