#ifndef MAIN_HPP
#define MAIN_HPP

#include <memory>
#include <mutex>
#include <string>
#include <unordered_map>

class Engine;
class Player;
class Zone;
class World;

/**
 * Engine class represents driver-owned runtime services for the MUD server.
 * A concrete engine may host an in-game virtual machine, a command dispatcher,
 * schedulers, or other shared resources. The driver constructs it before the
 * World, so those resources remain available during world construction and
 * destruction. The Engine class can be extended or replaced as needed.
 *
 * The transport layer (mudmux) already provides an asynchronous event loop, so
 * Engine does not implement one. It defines the boundary between the engine
 * and the world: a DikuMUD-style implementation may manage compiled data and
 * logic directly, while an LPMud-style implementation may host a virtual
 * machine that lets a mudlib bridge transport hooks and the game world.
 */
class Engine {
public:
    Engine();
    virtual ~Engine();

    // ---------------------------------------------------------
    // transport-layer callbacks for mudmux (see mudmux/hooks.h)
    // ---------------------------------------------------------
    static int on_connect(void* ctx, int slot, void* data, size_t data_len);
    static int on_disconnect(void* ctx, int slot, void* data, size_t data_len);
    // static int on_message_inbound(void* ctx, int slot, void* data, size_t data_len);
    // static int on_message_outbound(void* ctx, int slot, void* data, size_t data_len);
    static int on_transport_ready(void* ctx, int slot, void* data, size_t data_len);
    // static int on_prompt(void* ctx, int slot, void* data, size_t data_len);
    // static int on_telnet_subneg(void* ctx, int slot, void* data, size_t data_len);
    // static int on_timer(void* ctx, int slot, void* data, size_t data_len);
    // static int on_garbage_collection(void* ctx, int slot, void* data, size_t data_len);
};

/**
 * Player class represents a connected player in the MUD server, including their transport slot,
 * entry name, and current zone. It provides thread-safe access to the player state using a
 * recursive mutex. The Player class can be extended or replaced with a different implementation
 * if needed.
 */
class Player: public std::enable_shared_from_this<Player> {
public:
    using LogonType = Player; // type alias for the logon type, can be changed to a different class if needed

    Player (int slot, const std::string& entry_name) : slot_(slot), entry_name_(entry_name) {}
    virtual ~Player() = default;

    int slot() const;
    std::string entry_name() const;
    std::shared_ptr<Zone> current_zone() const;
    void set_current_zone(std::shared_ptr<Zone> zone);

    static void connect(int slot, std::shared_ptr<Player> player);
    static std::shared_ptr<Player> find_by_slot(int slot);
    static bool disconnect(int slot, const std::shared_ptr<Player>& player);

private:
    int slot_; // transport slot associated with this player
    std::string entry_name_; // entry name associated with this player
    std::shared_ptr<Zone> current_zone_; // current zone the player is in

    static std::recursive_mutex transports_mutex_; // mutex for the transports mapping
    static std::unordered_map<int, std::shared_ptr<Player>> transports_; // transport (slot -> Player) mapping

protected:
    mutable std::recursive_mutex mutex_; // mutex for player state
};

/**
 * Zone class represents a zone in the MUD server, which can contain rooms, objects, and other game elements.
 * It provides a base class for zone implementations and can be extended or replaced with a different
 * implementation if needed.
 *
 * Zones are reference-counted using std::shared_ptr, allowing for safe sharing of zone instances across
 * different parts of the server. A zone can be added to the World instance and retrieved by name, or referenced
 * by players (including private zones not added to the World instance).
 *
 * A zone can implement policy functions to control certain logic layer decisions, such as whether a player is
 * allowed to enter the zone. MUD systems can define their own policies with zone scope to make the logic layer
 * more flexible and extensible. In a DikuMUD-genre system, an AREA is naturally a zone with all the rooms, objects,
 * and mobs contained within it. In a LPMud-genre system, the driver does not have a concept of zones, but the mudlib
 * usually implements zones or "domain areas" to organize the game world. Policies can also be used in content
 * management decisions in LPMud-genre systems, to enforce certain security rules with wizard-level players.
 */
class Zone: public std::enable_shared_from_this<Zone> {
public:
    virtual ~Zone();

    virtual bool player_enter(const std::shared_ptr<Player>& player);
    virtual void player_leave(const std::shared_ptr<Player>& player);

    // Define policy functions for zone-specific logic decisions. These functions can be overridden in derived
    // classes or read settings from definition files to control the behavior of the zone.
    // Examples:
    // virtual bool policy_allows_player_entry(const std::shared_ptr<Player>& player) const = 0;
    // virtual bool policy_allows_spawn_mob(const std::string& mob_type) const = 0;
};

/**
 * World class represents a MUD server's game state, including zones and other
 * game-related data. It retains a non-owning reference to the Engine that
 * constructed it, and protects its own shared state with a recursive mutex.
 * The World class can be extended or replaced with a different implementation.
 */
class World {
public:
    explicit World(Engine& engine);
    virtual ~World() = default;

    void set_zone(std::string name, std::shared_ptr<Zone> zone);
    std::shared_ptr<Zone> get_zone(const std::string& name) const;
    bool remove_zone(const std::string& name);

protected:
    Engine& engine_; // non-owning; the engine must outlive this world

private:
    mutable std::recursive_mutex mutex_; // mutex for this world's shared state
    std::unordered_map<std::string, std::shared_ptr<Zone>> zones_; // mapping of zone names to Zone objects
};

#endif // MAIN_HPP
