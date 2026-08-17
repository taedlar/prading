# CMakeLists.txt
cmake_minimum_required(VERSION 3.28)

project(prading
    VERSION 1.0.0
    DESCRIPTION "An experimental MUD system that develop and extend itself with LLM."
    LANGUAGES CXX
)

# dependencies
find_package(spdlog QUIET)

# generate configuration header
include(GNUInstallDirs)
configure_file(
    ${CMAKE_CURRENT_SOURCE_DIR}/config.h.in
    ${CMAKE_CURRENT_BINARY_DIR}/config.h
)

# components
add_subdirectory(modules/mudmux)
add_subdirectory(src)
