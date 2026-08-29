# CMakeLists.txt
cmake_minimum_required(VERSION 3.28)

# project settings
project(prading
    VERSION 1.0.0
    DESCRIPTION "An experimental MUD system that develop and extend itself with LLM."
    LANGUAGES C CXX
)

include(CTest)
include(CMakeDependentOption)
cmake_dependent_option(BUILD_TESTING "Enable testing" ON "prading_IS_TOP_LEVEL" OFF)
cmake_dependent_option(BUILD_EXAMPLES "Enable building of examples" ON "prading_IS_TOP_LEVEL" OFF)

# dependencies
include(FetchContent)
FetchContent_Declare(
    argparse
    GIT_REPOSITORY https://github.com/p-ranav/argparse.git
    GIT_TAG v3.2
    OVERRIDE_FIND_PACKAGE
)
FetchContent_Declare(
    cmark
    GIT_REPOSITORY https://github.com/commonmark/cmark.git
    GIT_TAG 0.31.2
    OVERRIDE_FIND_PACKAGE
)
set(BUILD_TESTING OFF)
set(BUILD_SHARED_LIBS OFF)
FetchContent_MakeAvailable(argparse cmark)
unset(BUILD_TESTING)
unset(BUILD_SHARED_LIBS)

find_package(OpenSSL REQUIRED COMPONENTS SSL Crypto) # for BIO

# build options

# generate configuration header
include(GNUInstallDirs)
configure_file(
    ${CMAKE_CURRENT_SOURCE_DIR}/config.h.in
    ${CMAKE_CURRENT_BINARY_DIR}/config.h
)

# components
add_subdirectory(modules/mudmux) # also fetches spdlog, yaml-cpp
find_package(spdlog REQUIRED)
find_package(yaml-cpp REQUIRED)

add_subdirectory(lib/sqlite3)
add_subdirectory(lib/player-store)
add_subdirectory(src)

# tests
if(BUILD_TESTING)
    enable_testing()
    find_package(GTest REQUIRED)
    add_subdirectory(tests)
endif()

# examples
if(BUILD_EXAMPLES)

endif()
