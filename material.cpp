#include "material.hpp"
#include <unordered_map>

TileState::TileState()
    : moisture(0.0f), ice_fraction(0.0f), temperature(293.15f), nutrients_N(0.5f),
      nutrients_P(0.5f), nutrients_K(0.5f), organic_matter(0.1f), pH(7.0f),
      compaction(0.0f), structural_integrity(1.0f), snow_depth(0.0f),
      litter_mass(0.0f), contamination(0.0f), sediment(0.0f) {}

const MaterialProperties &get_material_properties(MaterialType type) {
    static const std::unordered_map<MaterialType, MaterialProperties> catalogue = {
        {MaterialType::AIR,
         {
             .name = "Air",
             .glyph = ' ',
             .fg_color = 7,
             .bg_color = 0,
             .density_kgm3 = 1.2f,
             .hardness = 0.0f,
             .compaction_resistance = 1.0f,
             .max_porosity = 1.0f,
             .permeability = 100.0f,
             .thermal_conductivity = 0.024f,
             .specific_heat = 1006.0f,
             .shear_strength = 0.0f,
             .fertility_base = 0.0f,
             .pH_base = 7.0f,
             .is_solid = false,
             .is_liquid = false,
             .is_opaque = false,
             .erodibility = 0.0f
         }},
        {MaterialType::WATER_FRESH,
         {
             .name = "Fresh Water",
             .glyph = '~',
             .fg_color = 4,
             .bg_color = 0,
             .density_kgm3 = 1000.0f,
             .hardness = 0.0f,
             .compaction_resistance = 1.0f,
             .max_porosity = 1.0f,
             .permeability = 0.01f,
             .thermal_conductivity = 0.6f,
             .specific_heat = 4184.0f,
             .shear_strength = 0.0f,
             .fertility_base = 0.0f,
             .pH_base = 7.0f,
             .is_solid = false,
             .is_liquid = true,
             .is_opaque = false,
             .erodibility = 0.0f
         }},
        {MaterialType::SOIL_BASE,
         {
             .name = "Base Soil",
             .glyph = '.',
             .fg_color = 3,
             .bg_color = 0,
             .density_kgm3 = 1500.0f,
             .hardness = 0.1f,
             .compaction_resistance = 0.3f,
             .max_porosity = 0.4f,
             .permeability = 0.00001f,
             .thermal_conductivity = 0.8f,
             .specific_heat = 850.0f,
             .shear_strength = 40.0f,
             .fertility_base = 0.5f,
             .pH_base = 7.0f,
             .is_solid = true,
             .is_liquid = false,
             .is_opaque = true,
             .erodibility = 0.5f
         }},
        {MaterialType::STONE_BASE,
         {
             .name = "Base Stone",
             .glyph = '.',
             .fg_color = 8,
             .bg_color = 0,
             .density_kgm3 = 2600.0f,
             .hardness = 0.8f,
             .compaction_resistance = 1.0f,
             .max_porosity = 0.05f,
             .permeability = 0.000001f,
             .thermal_conductivity = 2.0f,
             .specific_heat = 800.0f,
             .shear_strength = 150000.0f,
             .fertility_base = 0.0f,
             .pH_base = 7.5f,
             .is_solid = true,
             .is_liquid = false,
             .is_opaque = true,
             .erodibility = 0.02f
         }}
    };
  auto it = catalogue.find(type);
  if (it != catalogue.end()) {
    return it->second;
  }
  return catalogue.at(MaterialType::AIR);
}

const MaterialProperties* get_material_db() {
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

const MaterialProperties* material_db = get_material_db();
