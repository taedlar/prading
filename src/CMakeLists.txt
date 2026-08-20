# src/CMakeLists.txt

add_executable(prading main.cpp
    main.hpp
    connection.cpp
)

target_include_directories(prading PRIVATE
    ${CMAKE_BINARY_DIR} # for config.h
)
target_link_libraries(prading PRIVATE
    argparse::argparse
    mudmux::mudmux
    spdlog::spdlog
    OpenSSL::SSL
)

add_custom_command(TARGET prading POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy -t $<TARGET_FILE_DIR:prading> $<TARGET_RUNTIME_DLLS:prading>
    COMMAND_EXPAND_LISTS
    COMMENT "Copying runtime dependencies for prading"
)
