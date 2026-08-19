#include "config.h"
#include "main.hpp"

extern "C" int on_connect (void* ctx, int slot, void* data, size_t data_len) {
    std::string entry_name(static_cast<char*>(data), data_len);
    SPDLOG_INFO("new user connected on slot {}: {}", slot, entry_name);

    return 0;
}

extern "C" int on_transport_ready (void* ctx, int slot, void* data, size_t data_len) {
    SPDLOG_INFO("transport ready on slot {}", slot);

    return 0;
}

extern "C" int on_disconnect (void* ctx, int slot, void* data, size_t data_len) {
    SPDLOG_INFO("user disconnected on slot {}", slot);

    return 0;
}
