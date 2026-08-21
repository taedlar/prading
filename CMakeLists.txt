# CMakeLists.txt
cmake_minimum_required(VERSION 3.28)

# project settings
project(prading
    VERSION 1.0.0
    DESCRIPTION "An experimental MUD system that develop and extend itself with LLM."
    LANGUAGES C CXX
)
option(BUILD_TESTING "Enable testing" ON)
option(BUILD_EXAMPLES "Enable building of examples" ON)

# dependencies
include(FetchContent)
FetchContent_Declare(
    argparse
    GIT_REPOSITORY https://github.com/p-ranav/argparse.git
    GIT_TAG v3.2
    OVERRIDE_FIND_PACKAGE
)
FetchContent_MakeAvailable(argparse)

find_package(OpenSSL REQUIRED COMPONENTS SSL Crypto) # for BIO

# generate configuration header
include(GNUInstallDirs)
configure_file(
    ${CMAKE_CURRENT_SOURCE_DIR}/config.h.in
    ${CMAKE_CURRENT_BINARY_DIR}/config.h
)

# components
set(MUDMUX_BUILD_EXAMPLES OFF CACHE BOOL "Disable building of mudmux examples" FORCE)
add_subdirectory(modules/mudmux) # also fetches spdlog, yaml-cpp
find_package(spdlog REQUIRED)
find_package(yaml-cpp REQUIRED)

add_subdirectory(lib/sqlite3)
add_subdirectory(src)

# tests
include(CTest)
if(BUILD_TESTING)
    enable_testing()
    find_package(GTest REQUIRED)
endif()

# examples
if(BUILD_EXAMPLES)
endif()
