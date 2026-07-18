idf_component_register(
    SRCS "usb_device.c"

    INCLUDE_DIRS "./"
                 "../shell"
                 "../shell/letter-shell"
                 "../shell/letter-shell/src"

    REQUIRES
        esp_partition
        esp_tinyusb
        esp_usb_cdc_rom_console
        rtam
        vfs
        storage
)

# target_compile_options(${COMPONENT_LIB}
#                         PRIVATE
#                         -Wno-unused-const-variable
#                         -DRTAM_CFG_USER="rtam_cfg_user.h")
target_link_libraries(
    ${COMPONENT_LIB}
    
    INTERFACE
        "-u rtAppusb_device"
        "-u shellCommandswtich_log"
)
