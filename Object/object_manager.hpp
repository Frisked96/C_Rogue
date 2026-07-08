#pragma once
#include "event_bus.hpp"
#include "object_instance.hpp"
#include "object_prototype_db.hpp"
#include "../spatial_grid.hpp"
#include <deque>
#include <vector>

class Game_map;

// ObjectManager: Owns all ObjectInstances, manages their lifecycle,
// and runs tiered tick updates. Uses EventBus for subscribe/announce.
class ObjectManager {
private:
    // Generational slot for safe UID reuse
    struct Slot {
        ObjectInstance obj;
        uint32_t generation = 0;
        bool occupied = false;
    };

    std::vector<Slot> slots_;
    std::deque<uint32_t> free_slots_;
    std::vector<ObjectUID> death_queue_;

    ObjectPrototypeDB& proto_db_;
    EventBus& event_bus_;
    SpatialGrid<ObjectUID> spatial_grid_;

    uint64_t turn_counter_ = 0;

    // UID encoding: upper 16 bits = generation, lower 16 bits = index
    static ObjectUID make_uid(uint32_t index, uint32_t gen) {
        return (gen << 16) | (index & 0xFFFF);
    }
    static uint32_t uid_index(ObjectUID uid) { return uid & 0xFFFF; }
    static uint32_t uid_gen(ObjectUID uid)   { return uid >> 16; }

    // Internal tick helpers
    void tick_high(ObjectInstance& obj, const ObjectPrototype& proto);
    void tick_medium(ObjectInstance& obj, const ObjectPrototype& proto);
    void tick_low(ObjectInstance& obj, const ObjectPrototype& proto);
    void tick_vegetation(ObjectInstance& obj, const ObjectPrototype& proto, Game_map* map, bool is_world_gen);

public:
    ObjectManager(ObjectPrototypeDB& db, EventBus& bus);

    // Spawn a new object from a prototype ID at position (x,y,z).
    ObjectUID spawn(uint16_t prototype_id, int x, int y, int z);

    // Kill/remove an object.
    void kill(ObjectUID uid, Game_map* map = nullptr);

    // Get a pointer to a live object (nullptr if invalid/dead).
    ObjectInstance* get(ObjectUID uid);
    const ObjectInstance* get(ObjectUID uid) const;

    // Move an object to a new position.
    void move(ObjectUID uid, int nx, int ny, int nz);

    // Run one game turn. Advances turn_counter and ticks objects
    // at their appropriate frequencies.
    void tick(Game_map* map = nullptr, bool is_world_gen = false);

    // Get all active objects (for rendering/iteration).
    std::vector<ObjectInstance*> get_all_active();

    // Accessors
    const SpatialGrid<ObjectUID>& spatial() const { return spatial_grid_; }
    uint64_t current_turn() const { return turn_counter_; }
    const ObjectPrototypeDB& proto_db() const { return proto_db_; }
};
