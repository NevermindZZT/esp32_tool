idf_component_register(
    SRCS "misc.c"

    INCLUDE_DIRS "./"

    REQUIRES
        nvs_flash
        spi_flash
        esp_hw_support
        esp_wifi
        wpa_supplicant
        shell
)


target_link_libraries(
    ${COMPONENT_LIB}
    
    INTERFACE
        "-u shellCommandsysInfo"
        "-u shellCommandps"
)
