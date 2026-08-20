# CMakeLists.txt
cmake_minimum_required(VERSION 3.28)

project(prading
    VERSION 1.0.0
    DESCRIPTION "An experimental MUD system that develop and extend itself with LLM."
    LANGUAGES CXX
)

# dependencies
include(FetchContent)
FetchContent_Declare(
    argparse
    GIT_REPOSITORY https://github.com/p-ranav/argparse.git
    GIT_TAG v3.2
    FIND_PACKAGE_ARGS
        NAMES argparse
)
FetchContent_MakeAvailable(argparse)


# generate configuration header
include(GNUInstallDirs)
configure_file(
    ${CMAKE_CURRENT_SOURCE_DIR}/config.h.in
    ${CMAKE_CURRENT_BINARY_DIR}/config.h
)

# components
set(MUDMUX_BUILD_EXAMPLES OFF CACHE BOOL "Disable building of mudmux examples" FORCE)
add_subdirectory(modules/mudmux) # also fetches spdlog, yaml-cpp, OpenSSL
find_package(spdlog REQUIRED)
find_package(yaml-cpp REQUIRED)
find_package(OpenSSL REQUIRED)

add_subdirectory(src)
