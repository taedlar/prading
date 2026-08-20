#include "config.h"
#include "main.hpp"

extern "C" int on_connect (void* ctx, int slot, void* data, size_t data_len) {
    std::string entry_name(static_cast<char*>(data), data_len);
    SPDLOG_INFO("new user connected on slot {}: {}", slot, entry_name);

    {
        std::lock_guard<std::recursive_mutex> lock(Player::transports_mutex);
        Player::transports[slot] = std::make_shared<Player::LogonType>(slot, entry_name);
    }

    // TODO: enable transport layer protocols here (e.g., TLS, WebSocket, etc.) if needed

    return 0;
}

extern "C" int on_transport_ready (void* ctx, int slot, void* data, size_t data_len) {
    SPDLOG_INFO("transport ready on slot {}", slot);

    // TODO: send a welcome message and/or perform login sequence here if needed

    return 0;
}

extern "C" int on_disconnect (void* ctx, int slot, void* data, size_t data_len) {
    SPDLOG_INFO("user disconnected on slot {}", slot);

    // TODO: perform user data save/cleanup here if needed

    return 0;
}
