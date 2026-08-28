#include "config.h"
#include "main.hpp"

World::World(Engine& engine) : engine_(engine) {}

void World::set_zone(std::string name, std::shared_ptr<Zone> zone) {
    std::lock_guard<std::recursive_mutex> lock(mutex_);
    zones_[std::move(name)] = std::move(zone);
}

std::shared_ptr<Zone> World::get_zone(const std::string& name) const {
    std::lock_guard<std::recursive_mutex> lock(mutex_);
    const auto it = zones_.find(name);
    return it == zones_.end() ? nullptr : it->second;
}

bool World::remove_zone(const std::string& name) {
    std::lock_guard<std::recursive_mutex> lock(mutex_);
    return zones_.erase(name) != 0;
}
