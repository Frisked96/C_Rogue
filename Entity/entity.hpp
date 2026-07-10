#pragma once
#include "entity_properties.hpp"
#include <cstdint>

using EntityID = uint32_t;
const EntityID INVALID_ENTITY_ID = 0;

struct EntityState {
  // --- Spatial (Discrete grid position) ---
  int x, y, z;

  // --- Physical (Mutable, can deviate from defaults) ---
  float height; // meters
  float weight; // kg

  // --- Physiological Status (0 to 1 normalized, or absolute units) ---
  float hunger; // 0 = full, 1 = starving
  float thirst; // 0 = hydrated, 1 = parched

  float energy_stored;    // Current calories in reserves
  float stomach_contents; // Food waiting for digestion
  float excretion_buffer; // Accumulated waste

  // --- Rates (Units per second, adjusted by activity/environment) ---
  float hunger_rate;           // metabolic drain
  float thirst_rate;           // hydration drain
  float digestion_rate;        // stomach -> energy/excretion
  float excretion_making_rate; // digestion output -> waste

  // --- Camera (For viewport centering) ---
  struct {
    bool active = false;
  } camera;

  // --- Movement Intent ---
  bool has_intent_to_move = false;
  int intent_dx = 0;
  int intent_dy = 0;
  int intent_dz = 0;
};

struct Entity {
  EntityID id;
  EntityType type;
  EntityState state;
  bool active;

  const EntityProperties &props() const { return get_entity_properties(type); }
};
