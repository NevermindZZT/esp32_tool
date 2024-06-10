idf_component_register(
    SRCS "gui.c"
         "gui_common.c"
         "gui_fs.c"

    INCLUDE_DIRS "./"

    REQUIRES
        driver
        vfs
        lvgl
        lvgl_esp32_drivers
        rtam
        shell
        storage
        key
)

target_compile_options(${COMPONENT_LIB}
                        PRIVATE
                        -DRTAM_CFG_USER="rtam_cfg_user.h")
target_link_libraries(${COMPONENT_LIB} INTERFACE "-u rtAppgui")
