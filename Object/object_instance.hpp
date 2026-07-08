#pragma once
#include "life_state.hpp"
#include "object_prototype.hpp"
#include <cstdint>
#include <memory>
#include <vector>

using ObjectUID = uint32_t;
constexpr ObjectUID INVALID_OBJECT_UID = 0;

struct VegetationState {
    float growth = 0.1f;         // 0.0 (seedling) to 1.0 (fully grown)
    float moisture = 0.5f;       // 0.0 (dried out) to 1.0 (fully saturated)
    int fruit_cooldown = 0;      // turns until it can produce fruit/seeds
    int age = 0;                 // years active
    float water_damage = 0.0f;
    float canopy = 1.0f;
    float canopy_density = 0.1f;
    float current_flow_blockage = 0.0f;
};

// ObjectInstance: The live, mutable state in the world.
// Analogous to TileState for voxels -- only stores what changes.
struct ObjectInstance {
    ObjectUID uid = INVALID_OBJECT_UID;
    uint16_t prototype_id = 0;

    // Spatial (grid-locked integers matching the 3D grid world)
    int x = 0, y = 0, z = 0;

    // Vitality
    float health = 0.0f;
    uint32_t owner_uid = INVALID_OBJECT_UID;  // For economy/theft logic

    // Living State (Sparse Pointer)
    // Only allocated if prototype->is_living is true.
    std::unique_ptr<LifeState> life;

    // Vegetation State (Sparse Pointer)
    // Only allocated if prototype->behavior == ObjectBehavior::VEGETATION.
    std::unique_ptr<VegetationState> vegetation;

    // Simple inventory: UIDs of contained objects
    // Only used if prototype->is_container is true.
    std::vector<ObjectUID> inventory;

    // Is this instance currently active in the world?
    bool active = false;
};
