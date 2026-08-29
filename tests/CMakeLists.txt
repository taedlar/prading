# tests/CMakeLists.txt

add_executable(prading_units
    player_store_tests.cpp
)
target_link_libraries(prading_units PRIVATE
    player-store
    spdlog::spdlog
    GTest::gtest_main
)
target_include_directories(prading_units PRIVATE
    ${CMAKE_BINARY_DIR} # for config.h
)

include(GoogleTest)
gtest_discover_tests(prading_units)
