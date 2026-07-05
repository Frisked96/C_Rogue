#pragma once
#include "object_enums.hpp"
#include "../material.hpp"
#include <cstdint>
#include <string>

struct SpawnRequirements {
    bool requires_ground = true;
    MaterialType ground_material = MaterialType::SOIL_BASE;
    float min_soil_moisture = 0.05f; // min liquid_volume of the ground tile
    float max_soil_moisture = 1.0f;
    float min_temp = 278.0f;        // in Kelvin
    float max_temp = 320.0f;
    int min_z = 0;
    int max_z = 100;
    float probability = 0.0f;      // Probability of spawning on a valid tile (0 by default)
};

// ObjectPrototype: The static, read-only template.
// Stored in a central database. Defines WHAT an object IS.
// Analogous to MaterialProperties for tiles.
struct ObjectPrototype {
    uint16_t id;
    std::string name;
    char glyph;
    int fg_color;

    // Physical Properties
    float mass_kg;
    float max_health;
    MaterialType primary_material;

    // Capabilities (Flags)
    bool is_living;     // Does it need a LifeState ticker?
    bool is_static;     // Trees/Rocks (true) vs NPCs/Items (false)
    bool is_container;  // Can it hold other objects?
    bool is_blocking;   // Does it block movement?

    // Economy/Social
    int base_value;     // Monetary value
    ProfessionType default_job;

    // Tick frequency for updates
    TickFrequency tick_freq;

    // Behavior type — determines which tick system updates this object.
    // INERT objects just decay. VEGETATION absorbs water, grows, etc.
    ObjectBehavior behavior = ObjectBehavior::INERT;
    SpawnRequirements spawn_req;
};

