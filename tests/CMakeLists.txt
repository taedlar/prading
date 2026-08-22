add_executable(player-store-tests
    player_store_tests.cpp
)
target_link_libraries(player-store-tests PRIVATE
    player-store
    GTest::gtest_main
)
include(GoogleTest)
gtest_discover_tests(player-store-tests)