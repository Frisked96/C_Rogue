#include "material.hpp"
#include <unordered_map>

TileState::TileState()
    : moisture(0.0f), ice_fraction(0.0f), temperature(293.15f), nutrients_N(0.5f),
      nutrients_P(0.5f), nutrients_K(0.5f), organic_matter(0.1f), pH(7.0f),
      compaction(0.0f), structural_integrity(1.0f), snow_depth(0.0f),
      litter_mass(0.0f), contamination(0.0f) {}

const MaterialProperties &get_material_properties(MaterialType type) {
  static const std::unordered_map<MaterialType, MaterialProperties> catalogue = {
      {MaterialType::AIR,
       {"Air", ' ', 7, 0, 1.2f, 0.0f, 1.0f, 1.0f, 100.0f, 0.024f, 1006.0f, 0.0f,
        0.0f, 7.0f, false, false, false}},
      {MaterialType::WATER_FRESH,
       {"Fresh Water", '.', 4, 0, 1000.0f, 0.0f, 1.0f, 1.0f, 0.01f, 0.6f, 4184.0f,
        0.0f, 0.0f, 7.0f, false, true, false}},
      {MaterialType::STONE_GRANITE,
       {"Granite", '.', 8, 0, 2700.0f, 0.9f, 1.0f, 0.02f, 0.0f, 2.5f, 790.0f,
        200000.0f, 0.0f, 7.0f, true, false, true}},
      {MaterialType::SOIL_LOAM,
       {"Loam", '.', 3, 0, 1400.0f, 0.15f, 0.4f, 0.45f, 0.00001f, 1.0f, 800.0f,
        50.0f, 0.8f, 6.5f, true, false, true}},
      {MaterialType::SOIL_SAND,
       {"Sand", '.', 14, 0, 1600.0f, 0.1f, 0.1f, 0.35f, 0.001f, 0.27f, 830.0f,
        10.0f, 0.1f, 7.5f, true, false, true}},
      {MaterialType::SOIL_CLAY,
       {"Clay", '.', 6, 0, 1700.0f, 0.2f, 0.6f, 0.4f, 0.0000001f, 1.1f, 900.0f,
        100.0f, 0.5f, 6.0f, true, false, true}},
      {MaterialType::STONE_LIMESTONE,
       {"Limestone", '.', 7, 0, 2500.0f, 0.6f, 1.0f, 0.1f, 0.00001f, 1.3f, 900.0f,
        100000.0f, 0.0f, 8.5f, true, false, true}},
      {MaterialType::ORE_IRON,
       {"Iron Ore", '.', 1, 0, 5000.0f, 0.8f, 1.0f, 0.05f, 0.0f, 15.0f, 450.0f,
        300000.0f, 0.0f, 7.0f, true, false, true}},
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
