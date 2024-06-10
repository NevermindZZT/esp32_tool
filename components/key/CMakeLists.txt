idf_component_register(
    SRCS "key.c"
    INCLUDE_DIRS "./"

    REQUIRES
        driver
        rtam
)

target_link_libraries(
    ${COMPONENT_LIB}
    
    INTERFACE
        "-u rtAppkey"
)
