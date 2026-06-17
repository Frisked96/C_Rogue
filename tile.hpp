#pragma once

#include "material.hpp"
#include <cstdint>
#include <string>

// Minimal per‑voxel physics state – only what changes every frame for every
// tile. Soil chemistry, snow cover, structural integrity, etc. are stored
// sparsely in Game_map to save memory.
struct TileState {
  float liquid_volume = 0.0f;  // volume fraction of liquid water in pores
  float frozen_volume = 0.0f;  // volume fraction of ice in pores
  float temperature = 293.15f; // Kelvin
  float compaction = 0.0f;     // 0 = loose, 1 = fully compacted
};

struct Tile {
  static constexpr float WIDTH = 1.0f;
  static constexpr float HEIGHT = 1.0f;
  static constexpr float DEPTH = 1.0f;

  MaterialType material = MaterialType::AIR;
  TileState state;
  uint32_t object_id = 0;
  bool is_explored = false;
  bool is_visible = false;

  // Construction
  Tile() = default;
  explicit Tile(MaterialType m) : material(m) {}

  // Fast cached property lookup
  const MaterialProperties &mat() const noexcept {
    return material_db[static_cast<uint8_t>(material)];
  }
  const MaterialProperties &get_properties() const noexcept { return mat(); }

  // Geometric helpers
  float get_volume() const noexcept { return WIDTH * HEIGHT * DEPTH; }
  float get_mass() const noexcept { return mat().density_kgm3 * get_volume(); }
  float get_face_area() const noexcept { return WIDTH * HEIGHT; }

  // Porous‑media derived quantities
  float effective_porosity() const noexcept {
    return mat().max_porosity * (1.0f - 0.8f * state.compaction);
  }
  float water_capacity() const noexcept { return effective_porosity(); }
  float field_capacity() const noexcept { return mat().field_capacity; }
  float wilting_point() const noexcept { return mat().wilting_point; }
  float get_available_pore_volume() const noexcept {
    return get_volume() * effective_porosity();
  }

  // Plant growth checks are now handled by Game_map because they
  // depend on externally stored soil chemistry (N/P/K, pH, etc.)

  // Per‑turn updates
  void updateTemperature(float ambient, float neighbours[6]);
  void updateLiquidVolume(float rainfall, float drainage, float evaporation);
  void applyTrample(float weight, float area);

  // Query helpers
  bool is_wall() const noexcept {
    auto &p = mat();
    return p.is_solid && p.is_opaque;
  }
  bool is_floor() const noexcept {
    auto &p = mat();
    return !p.is_solid && !p.is_opaque;
  }
  char get_glyph() const noexcept { return mat().glyph; }
  bool get_is_walkable() const noexcept { return !mat().is_solid; }
  bool get_is_transparent() const noexcept { return !mat().is_opaque; }
  std::string get_name() const noexcept { return mat().name; }
};

namespace Tiles {
extern const Tile Wall;
extern const Tile Floor;
} // namespace Tiles