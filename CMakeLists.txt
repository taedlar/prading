# CMakeLists.txt
cmake_minimum_required(VERSION 3.28)

project(prading VERSION 1.0.0 LANGUAGES CXX)

include(FetchContent)
FetchContent_Declare(
    mudmux
    GIT_REPOSITORY https://github.com/taedlar/mudmux.git
    GIT_TAG main
)
FetchContent_MakeAvailable(mudmux)

add_subdirectory(src)
