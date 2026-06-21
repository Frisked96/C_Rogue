#pragma once
#include "object_enums.hpp"
#include "../material.hpp"
#include <cstdint>
#include <string>

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
};
