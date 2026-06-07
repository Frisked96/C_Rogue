#pragma once
#include <string>
#include <unordered_map>
#include <cstdint>

enum class EntityType : uint8_t {
    PLAYER,
    NPC_HUMANOID,
    ANIMAL_RABBIT,
    ANIMAL_WOLF,
};

struct EntityProperties {
    std::string name;
    char glyph;
    int fg_color;
    
    // --- Basic Data (Static/Spawn Defaults) ---
    float base_height;       // meters
    float base_weight;       // kg
    
    // --- Physiological Parameters (Base Rates) ---
    float daily_calorie_need;  // Calories per 24h at rest
    float daily_water_need;    // Units of water per 24h at rest
    
    float digestion_rate;      // Units per second
    float excretion_making_rate; // Rate of converting food to waste
    
    // --- Capacity Limits ---
    float max_stomach;         // Max food units
    float max_calories_stored; // Max energy storage
    float max_excretion_buffer; // Max waste storage before needing relief
    float max_thirst_buffer;   // Max hydration level
    
    int vision_radius;         // How far the entity can see
};

const EntityProperties& get_entity_properties(EntityType type);
