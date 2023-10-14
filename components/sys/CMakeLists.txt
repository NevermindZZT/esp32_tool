# idf_component_register(
#     SRCS "station.c" "scan.c" "smartconfig.c" "misc.c"
#     INCLUDE_DIRS "./"
#                  "../shell"
#                  "../shell/src"
#     REQUIRES "tcpip_adapter"
#              "spi_flash"
#              "wpa_supplicant"
#              "lwip"
# )
idf_component_register(
    SRCS "misc.c"
    INCLUDE_DIRS "./"
                 "../shell"
                 "../shell/letter-shell"
                 "../shell/letter-shell/src"
    REQUIRES "spi_flash"
             "esp_hw_support"
)