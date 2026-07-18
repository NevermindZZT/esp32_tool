idf_component_register(
    SRCS "ble_sniffer.c"
         "ble_sniffer_pcap.c"
         "icon_app_ble_sniffer.c"

    INCLUDE_DIRS "./"

    REQUIRES
        bt
        bt_manager
        esp_tinyusb
        gui
        launcher
        rtam
        shell
)

target_compile_options(${COMPONENT_LIB} PRIVATE
    -Wno-cast-function-type)

target_link_libraries(
    ${COMPONENT_LIB}

    INTERFACE
        "-u rtAppble_sniff"
        "-u shellCommandble_sniff"
)
