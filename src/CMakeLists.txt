# src/CMakeLists.txt

add_executable(prading main.cpp)
target_link_libraries(prading PRIVATE mudmux::mudmux)
