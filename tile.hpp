#pragma once

#include "material.hpp"
#include <string>

struct Tile {
  MaterialType material;
  TileState state;
  uint32_t object_id; // 0 = no object
  bool is_explored;

  // Constructor
  Tile() : material(MaterialType::AIR), object_id(0), is_explored(false) {}
  Tile(MaterialType m) : material(m), object_id(0), is_explored(false) {}

  // --- Cached lookups for speed ---
  const MaterialProperties &mat() const {
    return material_db[static_cast<uint8_t>(material)];
  }

  const MaterialProperties &get_properties() const { return mat(); }

  // --- Frequently used derived values ---
  float effective_porosity() const {
    if (material == MaterialType::AIR)
      return 1.0f;
    // compaction linearly reduces porosity, but never below 20% of max
    return mat().max_porosity * (1.0f - 0.8f * state.compaction);
  }

  bool can_plant() const {
    return mat().fertility_base > 0.0f && state.moisture > 0.05f &&
           state.temperature > 278.0f;
  }

  // --- update methods called each turn ---
  void updateTemperature(float ambient, float neighbours[4]); // 4-way for 2D
  void updateMoisture(float rainfall, float drainage, float evaporation);
  void applyTrample(float weight, float area);

  // Utility methods
  bool is_wall() const {
    const auto &prop = mat();
    return prop.is_solid && prop.is_opaque;
  }
  bool is_floor() const {
    const auto &prop = mat();
    return !prop.is_solid && !prop.is_opaque;
  }

  // Backward compatibility getters
  char get_glyph() const { return mat().glyph; }
  bool get_is_walkable() const { return !mat().is_solid; }
  bool get_is_transparent() const { return !mat().is_opaque; }
  std::string get_name() const { return mat().name; }
};

namespace Tiles {
extern const Tile Wall;
extern const Tile Floor;
} // namespace Tiles
