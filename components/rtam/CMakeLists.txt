idf_component_register(
    SRCS "rtam/src/rtam.c"

    INCLUDE_DIRS "./"
                 "./rtam/src/"
                 "../shell"
                 "../shell/letter-shell"
                 "../shell/letter-shell/src"
                 "../lvgl"

    REQUIRES
        driver

    LDFRAGMENTS "rtam.lf"
)

target_compile_options(${COMPONENT_LIB}
                        PRIVATE
                        -Wno-unused-const-variable
                        -DRTAM_CFG_USER="rtam_cfg_user.h")
