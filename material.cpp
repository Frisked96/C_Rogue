#include "material.hpp"
#include <unordered_map>

TileState::TileState()
    : moisture(0.0f), ice_fraction(0.0f), temperature(293.15f),
      nutrients_N(0.5f), nutrients_P(0.5f), nutrients_K(0.5f),
      organic_matter(0.1f), pH(7.0f), compaction(0.0f),
      structural_integrity(1.0f), snow_depth(0.0f), litter_mass(0.0f),
      contamination(0.0f), sediment(0.0f) {}

const MaterialProperties &get_material_properties(MaterialType type) {
  static const std::unordered_map<MaterialType, MaterialProperties> catalogue = []() {
    std::unordered_map<MaterialType, MaterialProperties> m;
    MaterialProperties p;
    p.name = "Air";
    p.glyph = ' ';
    p.fg_color = 7;
    p.bg_color = 0;
    p.density_kgm3 = 1.2f;
    p.hardness = 0.0f;
    p.compaction_resistance = 1.0f;
    p.max_porosity = 1.0f;
    p.permeability = 100.0f;
    p.field_capacity = 1.0f;
    p.wilting_point = 0.0f;
    p.thermal_conductivity = 0.024f;
    p.specific_heat = 1006.0f;
    p.shear_strength = 0.0f;
    p.fertility_base = 0.0f;
    p.pH_base = 7.0f;
    p.is_solid = false;
    p.is_liquid = false;
    p.is_opaque = false;
    p.erodibility = 0.0f;
    m.emplace(MaterialType::AIR, p);

    p.name = "Fresh Water";
    p.glyph = '~';
    p.fg_color = 4;
    p.bg_color = 0;
    p.density_kgm3 = 1000.0f;
    p.hardness = 0.0f;
    p.compaction_resistance = 1.0f;
    p.max_porosity = 1.0f;
    p.permeability = 0.01f;
    p.field_capacity = 1.0f;
    p.wilting_point = 0.0f;
    p.thermal_conductivity = 0.6f;
    p.specific_heat = 4184.0f;
    p.shear_strength = 0.0f;
    p.fertility_base = 0.0f;
    p.pH_base = 7.0f;
    p.is_solid = false;
    p.is_liquid = true;
    p.is_opaque = false;
    p.erodibility = 0.0f;
    m.emplace(MaterialType::WATER_FRESH, p);

    p.name = "Base Soil";
    p.glyph = '.';
    p.fg_color = 3;
    p.bg_color = 0;
    p.density_kgm3 = 1500.0f;
    p.hardness = 0.1f;
    p.compaction_resistance = 0.3f;
    p.max_porosity = 0.4f;
    p.permeability = 0.00001f;
    p.field_capacity = 0.2f;
    p.wilting_point = 0.08f;
    p.thermal_conductivity = 0.8f;
    p.specific_heat = 850.0f;
    p.shear_strength = 40.0f;
    p.fertility_base = 0.5f;
    p.pH_base = 7.0f;
    p.is_solid = true;
    p.is_liquid = false;
    p.is_opaque = true;
    p.erodibility = 0.5f;
    m.emplace(MaterialType::SOIL_BASE, p);

    p.name = "Base Stone";
    p.glyph = '.';
    p.fg_color = 8;
    p.bg_color = 0;
    p.density_kgm3 = 2600.0f;
    p.hardness = 0.8f;
    p.compaction_resistance = 1.0f;
    p.max_porosity = 0.05f;
    p.permeability = 0.000001f;
    p.field_capacity = 0.04f;
    p.wilting_point = 0.02f;
    p.thermal_conductivity = 2.0f;
    p.specific_heat = 800.0f;
    p.shear_strength = 150000.0f;
    p.fertility_base = 0.0f;
    p.pH_base = 7.5f;
    p.is_solid = true;
    p.is_liquid = false;
    p.is_opaque = true;
    p.erodibility = 0.02f;
    m.emplace(MaterialType::STONE_BASE, p);

    return m;
  }();
  auto it = catalogue.find(type);
  if (it != catalogue.end()) {
    return it->second;
  }
  return catalogue.at(MaterialType::AIR);
}

const MaterialProperties *get_material_db() {
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

const MaterialProperties *material_db = get_material_db();
