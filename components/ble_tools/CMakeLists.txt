idf_component_register(
    SRCS "ble_advert.c"

    INCLUDE_DIRS "./"

    REQUIRES
        bt
        bt_manager
        shell
)

target_compile_options(${COMPONENT_LIB} PRIVATE
    -Wno-cast-function-type)

target_link_libraries(
    ${COMPONENT_LIB}

    INTERFACE
        "-u shellCommandble_advert"
)
