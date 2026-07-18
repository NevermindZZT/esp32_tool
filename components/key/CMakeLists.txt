idf_component_register(
    SRCS "key.c"
    INCLUDE_DIRS "./"

    REQUIRES
        esp_driver_gpio
        rtam
)

target_link_libraries(
    ${COMPONENT_LIB}
    
    INTERFACE
        "-u rtAppkey"
)
