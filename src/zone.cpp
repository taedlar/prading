#include "config.h"
#include "main.hpp"

Zone::~Zone() {
    // A player holding a reference to this zone will keep it alive until they release it.
}

bool Zone::player_enter(const std::shared_ptr<Player>& player) {
    // Implement logic for a player entering the zone.
    // Return true if the player is allowed to enter, false otherwise.
    player->set_current_zone(shared_from_this());
    return true;
}

void Zone::player_leave(const std::shared_ptr<Player>& player) {
    // Implement logic for a player leaving the zone.
    player->set_current_zone(nullptr);
}
