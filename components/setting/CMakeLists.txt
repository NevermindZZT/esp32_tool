idf_component_register(
    SRCS "icon_app_setting_bt.c" "icon_app_setting_wifi.c" "setting_provider.c"
         "setting_brightness.c"
         "setting.c"
         "setting_common.c"
        
         "setting_about.c"
         "icon_app_setting.c"
        
         "icon_app_setting_about.c"
         "icon_app_setting_brightness.c"
         "icon_app_setting_wifi.c"
         "icon_app_setting_bt.c"

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
        nvs_flash
        cpost
)

target_compile_options(${COMPONENT_LIB} PRIVATE -DPROJECT_VER="${PROJECT_VER}")
target_link_libraries(${COMPONENT_LIB} INTERFACE "-u rtAppsetting")
