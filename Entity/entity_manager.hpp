#pragma once
#include <vector>
#include <deque>
#include <unordered_map>
#include "entity.hpp"
#include "spatial_grid.hpp"

class EntityManager {
private:
    struct Slot {
        Entity entity;
        uint32_t generation;
        bool active;
    };

    std::vector<Slot> slots;
    std::deque<uint32_t> free_slots;
    SpatialGrid spatial_grid;

    EntityID make_id(uint32_t index, uint32_t generation) const {
        return (generation << 16) | (index & 0xFFFF);
    }
    uint32_t get_index(EntityID id) const { return id & 0xFFFF; }
    uint32_t get_generation(EntityID id) const { return id >> 16; }

public:
    EntityManager();

    EntityID spawn(EntityType type, int x, int y, int z);
    void kill(EntityID id);

    Entity* get(EntityID id);
    const Entity* get(EntityID id) const;

    void update(float dt);
    
    // For renderer/iteration
    std::vector<Entity*> get_all_active();
    
    SpatialGrid& get_spatial_grid() { return spatial_grid; }
    const SpatialGrid& get_spatial_grid() const { return spatial_grid; }
};
