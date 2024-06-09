idf_component_register(
    SRCS "storage.c"
    INCLUDE_DIRS "./"

    REQUIRES
        vfs
        fatfs
        rtam
)

target_link_libraries(
    ${COMPONENT_LIB}
    
    INTERFACE
        "-u rtAppstorage"
        "-u shellCommandtest_file_read"
)
