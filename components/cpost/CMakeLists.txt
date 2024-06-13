idf_component_register(
    SRCS
        "cpost/src/cpost.c"
        "cpost_task.c"
    
    INCLUDE_DIRS
        "./"
        "./cpost/src/"

    REQUIRES
        rtam
)

target_compile_options(${COMPONENT_LIB} PRIVATE -DCPOST_CFG_USER="cpost_user_config.h")

target_link_libraries(
    ${COMPONENT_LIB}
    
    INTERFACE
        "-u rtAppcpost"
)
