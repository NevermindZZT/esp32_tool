idf_component_register(
    SRCS "letter-shell/src/shell.c"
         "letter-shell/src/shell_ext.c"
         "shell_port.c"

    INCLUDE_DIRS "./"
                 "./letter-shell/src"
                 "../"
                 "../rtam"
                 "../rtam/rtam/src"

    REQUIRES
        driver

    LDFRAGMENTS "letter-shell/demo/esp-idf/shell.lf"
)

target_compile_options(${COMPONENT_LIB} PRIVATE -DSHELL_CFG_USER="shell_cfg_user.h")
target_link_libraries(${COMPONENT_LIB} INTERFACE "-u rtAppshell")
