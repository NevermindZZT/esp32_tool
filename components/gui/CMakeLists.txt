idf_component_register(
    SRCS "gui.c"
         "gui_common.c"

    INCLUDE_DIRS "./"
                 "../shell"
                 "../shell/letter-shell"
                 "../shell/letter-shell/src"
                 "../lvgl"
                 "../lvgl_esp32_drivers"
                 "../rtam"
                 "../rtam/rtam/src"

    REQUIRES
        driver
        lvgl
        lvgl_esp32_drivers
)

target_compile_options(${COMPONENT_LIB}
                        PRIVATE
                        -DRTAM_CFG_USER="rtam_cfg_user.h")
target_link_libraries(${COMPONENT_LIB} INTERFACE "-u rtAppgui")
