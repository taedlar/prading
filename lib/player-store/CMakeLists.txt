add_library(player-store STATIC
    player_store.cpp
    include/player_store/player_store.hpp
)

target_include_directories(player-store PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}/include
)
target_link_libraries(player-store PUBLIC sqlite3)
target_compile_features(player-store PUBLIC cxx_std_17)