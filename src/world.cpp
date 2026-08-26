#include "config.h"
#include "main.hpp"

std::recursive_mutex World::world_mutex_;
std::unique_ptr<World::CosmosType> World::instance_ = nullptr;

void World::initialize() {
    std::lock_guard<std::recursive_mutex> lock(world_mutex_);
    instance_ = std::make_unique<CosmosType>();

    // Populate initial zones or perform other setup tasks for the world instance if needed.
}

World::CosmosType* World::get_instance() {
    std::lock_guard<std::recursive_mutex> lock(world_mutex_);
    return instance_.get();
}

void World::shutdown() {
    std::lock_guard<std::recursive_mutex> lock(world_mutex_);
    instance_.reset();
}

void World::set_zone(std::string name, std::shared_ptr<Zone> zone) {
    std::lock_guard<std::recursive_mutex> lock(world_mutex_);
    zones_[std::move(name)] = std::move(zone);
}

std::shared_ptr<Zone> World::get_zone(const std::string& name) const {
    std::lock_guard<std::recursive_mutex> lock(world_mutex_);
    const auto it = zones_.find(name);
    return it == zones_.end() ? nullptr : it->second;
}

bool World::remove_zone(const std::string& name) {
    std::lock_guard<std::recursive_mutex> lock(world_mutex_);
    return zones_.erase(name) != 0;
}
