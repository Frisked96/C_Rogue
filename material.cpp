// material.cpp
#include "material.hpp"
#include <unordered_map>

const MaterialProperties& get_material_properties(MaterialType type) noexcept {
    static const std::unordered_map<MaterialType, MaterialProperties> catalogue = []() {
        std::unordered_map<MaterialType, MaterialProperties> m;
        MaterialProperties p;

        // ---- AIR ----
        p.name                   = "Air";
        p.glyph                  = ' ';
        p.fg_color               = 7;
        p.density_kgm3           = 1.2f;
        p.max_porosity           = 1.0f;      // 100 % voids
        p.is_solid               = false;
        p.is_opaque              = false;
        p.permeability           = 100.0f;    // very high – no matrix resistance
        p.field_capacity         = 1.0f;      // capillary forces irrelevant
        p.wilting_point          = 0.0f;
        p.thermal_conductivity   = 0.024f;
        p.specific_heat          = 1006.0f;
        p.compaction_resistance  = 1.0f;      // cannot be compacted
        p.shear_strength         = 0.0f;
        m.emplace(MaterialType::AIR, p);

        // ---- SOIL_BASE ----
        p.name                   = "Base Soil";
        p.glyph                  = '.';
        p.fg_color               = 3;
        p.density_kgm3           = 1500.0f;
        p.max_porosity           = 0.4f;
        p.is_solid               = true;
        p.is_opaque              = true;
        p.permeability           = 0.00001f;
        p.field_capacity         = 0.2f;
        p.wilting_point          = 0.08f;
        p.thermal_conductivity   = 0.8f;
        p.specific_heat          = 850.0f;
        p.compaction_resistance  = 0.3f;
        p.shear_strength         = 40.0f;
        m.emplace(MaterialType::SOIL_BASE, p);

        // ---- STONE_BASE ----
        p.name                   = "Base Stone";
        p.glyph                  = '.';
        p.fg_color               = 8;
        p.density_kgm3           = 2600.0f;
        p.max_porosity           = 0.05f;
        p.is_solid               = true;
        p.is_opaque              = true;
        p.permeability           = 0.000001f;
        p.field_capacity         = 0.04f;
        p.wilting_point          = 0.02f;
        p.thermal_conductivity   = 2.0f;
        p.specific_heat          = 800.0f;
        p.compaction_resistance  = 1.0f;      // essentially incompressible
        p.shear_strength         = 150000.0f;
        m.emplace(MaterialType::STONE_BASE, p);

        return m;
    }();

    auto it = catalogue.find(type);
    return (it != catalogue.end()) ? it->second : catalogue.at(MaterialType::AIR);
}

// Build a full 256‑element array for fast indexing
static const MaterialProperties* build_material_db() {
    static MaterialProperties db[256];
    static bool initialized = false;
    if (!initialized) {
        for (int i = 0; i < 256; ++i) {
            db[i] = get_material_properties(static_cast<MaterialType>(i));
        }
        initialized = true;
    }
    return db;
}

const MaterialProperties* material_db = build_material_db();