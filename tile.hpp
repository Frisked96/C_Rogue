#pragma once

#include "material.hpp"
#include <string>

struct Tile {
  // Dimensions in meters
  static constexpr float WIDTH = 1.0f;
  static constexpr float HEIGHT = 1.0f;
  static constexpr float DEPTH = 1.0f;

  MaterialType material;
  TileState state;
  uint32_t object_id; // 0 = no object
  bool is_explored;
  bool is_visible;

  // Constructor
  Tile()
      : material(MaterialType::AIR), object_id(0), is_explored(false),
        is_visible(false) {}
  Tile(MaterialType m)
      : material(m), object_id(0), is_explored(false), is_visible(false) {}

  // --- Cached lookups for speed ---
  const MaterialProperties &mat() const {
    return material_db[static_cast<uint8_t>(material)];
  }

  const MaterialProperties &get_properties() const { return mat(); }

  // --- Physical properties ---
  float get_volume() const { return WIDTH * HEIGHT * DEPTH; }
  float get_mass() const { return mat().density_kgm3 * get_volume(); }
  float get_face_area() const { return WIDTH * HEIGHT; } // Assuming cubic

  // --- Frequently used derived values ---
  float effective_porosity() const {
    if (material == MaterialType::AIR)
      return 1.0f;
    // compaction linearly reduces porosity, but never below 20% of max
    return mat().max_porosity * (1.0f - 0.8f * state.compaction);
  }

  float get_available_pore_volume() const {
    return get_volume() * effective_porosity();
  }

  bool can_plant() const {
    return mat().fertility_base > 0.0f && state.moisture > 0.05f &&
           state.temperature > 278.0f;
  }

  // --- update methods called each turn ---
  void updateTemperature(float ambient, float neighbours[6]); // 6-way for 3D
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
