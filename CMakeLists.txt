# The following lines of boilerplate have to be in your project's
# CMakeLists in this exact order for cmake to work correctly
cmake_minimum_required(VERSION 3.16)

# Required for CMake >= 4.0 with cross-compilation toolchain:
# skip linker test during compiler ABI detection (picolibc sysroot is
# configured later by ESP-IDF's project.cmake)
set(CMAKE_TRY_COMPILE_TARGET_TYPE STATIC_LIBRARY)

include($ENV{IDF_PATH}/tools/cmake/project.cmake)
project(esp32-tool VERSION 0.0.1)

# set(PROJECT_VER 0.0.1)

idf_build_set_property(COMPILE_OPTIONS "-DSHELL_CFG_USER=\"shell_cfg_user.h\"" APPEND)
idf_build_set_property(COMPILE_OPTIONS "-DRTAM_CFG_USER=\"rtam_cfg_user.h\"" APPEND)
idf_build_set_property(COMPILE_OPTIONS "-DCPOST_CFG_USER=\"cpost_user_config.h\"" APPEND)