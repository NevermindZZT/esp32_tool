idf_component_register(
    SRCS "usb_device.c"

    INCLUDE_DIRS "./"
                 "../shell"
                 "../shell/letter-shell"
                 "../shell/letter-shell/src"

    REQUIRES
        driver
        esp_partition
        esp_tinyusb
        rtam
        vfs
)

# target_compile_options(${COMPONENT_LIB}
#                         PRIVATE
#                         -Wno-unused-const-variable
#                         -DRTAM_CFG_USER="rtam_cfg_user.h")
target_link_libraries(${COMPONENT_LIB} INTERFACE "-u rtAppusb_device")
