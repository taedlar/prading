#include "config.h"
#include "main.hpp"

std::recursive_mutex Player::transports_mutex_;
std::unordered_map<int, std::shared_ptr<Player>> Player::transports_;

int Player::slot() const {
    std::lock_guard<std::recursive_mutex> lock(mutex_);
    return slot_;
}

std::string Player::entry_name() const {
    std::lock_guard<std::recursive_mutex> lock(mutex_);
    return entry_name_;
}

std::shared_ptr<Zone> Player::current_zone() const {
    std::lock_guard<std::recursive_mutex> lock(mutex_);
    return current_zone_;
}

void Player::set_current_zone(std::shared_ptr<Zone> zone) {
    std::lock_guard<std::recursive_mutex> lock(mutex_);
    current_zone_ = std::move(zone);
}

void Player::connect(int slot, std::shared_ptr<Player> player) {
    std::lock_guard<std::recursive_mutex> lock(transports_mutex_);
    transports_[slot] = std::move(player);
}

std::shared_ptr<Player> Player::find_by_slot(int slot) {
    std::lock_guard<std::recursive_mutex> lock(transports_mutex_);
    const auto it = transports_.find(slot);
    return it == transports_.end() ? nullptr : it->second;
}

bool Player::disconnect(int slot, const std::shared_ptr<Player>& player) {
    std::lock_guard<std::recursive_mutex> lock(transports_mutex_);
    const auto it = transports_.find(slot);
    if (it == transports_.end() || it->second != player)
        return false;

    transports_.erase(it);
    return true;
}
