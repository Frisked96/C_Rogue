#pragma once

#include <cstdint>
#include <string>

enum class MaterialType : uint8_t {
    AIR,
    WATER_FRESH,
    WATER_SALT,
    SOIL_SAND,
    SOIL_LOAM,
    SOIL_CLAY,
    SOIL_SILT,
    SOIL_PEAT,
    STONE_GRANITE,
    STONE_LIMESTONE,
    STONE_SANDSTONE,
    STONE_BASALT,
    ORE_IRON,
    MAGMA,
    // ... expand as needed
};

struct MaterialProperties {
    std::string name;
    char glyph;
    int fg_color;
    int bg_color;

    float density_kgm3;          // mass per m³ (air ~1.2, water 1000, granite 2700)
    float hardness;              // 0–1, affects mining speed & tool wear
    float compaction_resistance; // 0–1, 0 = compacts instantly, 1 = never compacts
    float max_porosity;          // volume fraction available for water/air (0 for solid rock)
    float permeability;          // how fast water flows through saturated material, m/s per unit head
    float thermal_conductivity;  // W/(m·K)
    float specific_heat;         // J/(kg·K)
    float shear_strength;        // kPa, for cave‑in / stability calculations
    float fertility_base;        // 0–1, innate nutrient richness (only meaningful for soils)
    float pH_base;               // starting pH
    bool  is_solid;              // blocks movement & sight
    bool  is_liquid;             // flows, drowns
    bool  is_opaque;             // blocks light
};

struct TileState {
    // --- hydrological ---
    float moisture;          // 0 .. effective_porosity, volume fraction of liquid water
    float ice_fraction;      // 0–1, proportion of pore water that is frozen; 0 unless temperature<273K

    // --- thermal ---
    float temperature;       // Kelvin, e.g. 293.15 = 20°C

    // --- edaphic (soil) ---
    float nutrients_N;       // relative 0–1 (0 = depleted, 1 = optimal)
    float nutrients_P;
    float nutrients_K;
    float organic_matter;   // 0–1 fraction, affects water retention and nutrient regeneration
    float pH;               // 0–14

    // --- mechanical ---
    float compaction;       // 0 = loose, 1 = fully compacted (reduces porosity, infiltration, root penetration)
    float structural_integrity; // 1 = intact, 0 = collapsed (used for ceilings/walls)

    // --- surface cover ---
    float snow_depth;       // metres (0–1, since tile is 1 m tall)
    float litter_mass;      // kg of leaf litter / debris (fuels fire, decays into organic matter)

    // --- contamination ---
    float contamination;    // generic 0–1 poison level

    TileState(); // Initialize with defaults
};

const MaterialProperties& get_material_properties(MaterialType type);

// Simple database access for Tile class
extern const MaterialProperties* material_db;

