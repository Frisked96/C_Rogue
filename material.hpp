// material.hpp
#pragma once

#include <cstdint>
#include <string>

enum class MaterialType : uint8_t {
  AIR,
  SOIL_BASE,
  STONE_BASE,
  // … expand as needed
};

struct MaterialProperties {
  // ----- identification -----
  std::string name;
  char glyph;
  int fg_color;

  // ----- physical matrix -----
  float density_kgm3; // mass of the solid part (kg/m³)
  float max_porosity; // volume fraction of voids (0..1)
  bool is_solid;      // does the matrix block movement?
  bool is_opaque;     // does the matrix block light?

  // ----- hydrology (universal for porous media) -----
  float permeability;   // ease of fluid flow through voids (m/s per unit head)
  float field_capacity; // void fraction at which capillary drainage stops
  float wilting_point;  // void fraction below which plants cannot extract water

  // ----- thermodynamics -----
  float thermal_conductivity; // W/(m·K)
  float specific_heat;        // J/(kg·K)

  // ----- mechanics -----
  float compaction_resistance; // 0 = compacts instantly, 1 = never compacts
  float shear_strength;        // kPa, for cave‑in / stability calculations
};

// Accessor for a single material type.
const MaterialProperties &get_material_properties(MaterialType type) noexcept;

// Pre‑built lookup table (indexed by uint8_t cast of MaterialType).
extern const MaterialProperties *material_db;