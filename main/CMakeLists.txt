idf_component_register(
    SRCS "main.c"
         "export_keep.c"

    INCLUDE_DIRS "./"
                 "../components"

    REQUIRES
        nvs_flash
        shell
        hid_device
        sys
        rtam
        launcher
        setting
        test
)