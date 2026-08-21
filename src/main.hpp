#ifndef MAIN_HPP
#define MAIN_HPP

#include <memory>
#include <mutex>
#include <string>
#include <unordered_map>

// logic-layer base classes for the MUD server

class Zone: public std::enable_shared_from_this<Zone> {
public:
    virtual ~Zone() = default;
};

/**
 * World class represents the global state of the MUD server, including zones
 * and other game-related data. It provides thread-safe access to the world state
 * using a recursive mutex. The World class can be extended or replaced with a different
 * implementation if needed.
 */
class World {
protected:
    std::unordered_map<std::string, std::shared_ptr<Zone>> zones_; // mapping of zone names to Zone objects

public:
    using CosmosType = World; // type alias for the cosmos type, can be changed to a different class if needed

    static std::recursive_mutex world_mutex; // mutex for thread-safe access to the world state
    static std::unique_ptr<CosmosType> instance; // polymorphic global instance of the world state

    virtual ~World() = default;

};

class Player: public std::enable_shared_from_this<Player> {
protected:
    std::recursive_mutex mutex_; // mutex for thread-safe access to the player state
    int slot_; // transport slot associated with this player
    std::string entry_name_; // entry name associated with this player
    std::shared_ptr<Zone> current_zone_; // current zone the player is in

public:
    static std::recursive_mutex transports_mutex; // mutex for thread-safe access to the transports mapping
    static std::unordered_map<int, std::shared_ptr<Player>> transports; // transport (slot -> Player) mapping

    Player (int slot, const std::string& entry_name) : slot_(slot), entry_name_(entry_name) {}
    virtual ~Player() = default;

    using LogonType = Player; // type alias for the logon type, can be changed to a different class if needed
};

// transport-layer callbacks for mudmux
extern "C" int on_connect (void* ctx, int slot, void* data, size_t data_len);
extern "C" int on_transport_ready (void* ctx, int slot, void* data, size_t data_len);
extern "C" int on_disconnect (void* ctx, int slot, void* data, size_t data_len);

#endif // MAIN_HPP
