#include "config.h"
#include "main.hpp"

std::recursive_mutex Engine::engine_mutex_;
std::unique_ptr<Engine> Engine::instance_ = nullptr;

bool Engine::initialize() {
    std::lock_guard<std::recursive_mutex> lock(engine_mutex_);
    instance_ = std::make_unique<Engine>();

    // Perform any additional setup tasks for the engine instance if needed.
    return true;
}

Engine* Engine::get_instance() {
    std::lock_guard<std::recursive_mutex> lock(engine_mutex_);
    return instance_.get();
}

void Engine::shutdown() {
    std::lock_guard<std::recursive_mutex> lock(engine_mutex_);
    instance_.reset();
}
