idf_component_register(
    SRCS "battery.c"
    INCLUDE_DIRS "./"

    REQUIRES
        esp_driver_gpio
        esp_adc
        rtam
)

target_link_libraries(
    ${COMPONENT_LIB}
    
    INTERFACE
        "-u rtAppbattery"
        "-u shellCommandbattery"
)
