// tile.cpp
#include "tile.hpp"
#include <algorithm>
#include <cmath>

namespace Tiles {
const Tile Wall{MaterialType::STONE_BASE};
const Tile Floor{MaterialType::SOIL_BASE};
} // namespace Tiles

void Tile::updateTemperature(float ambient, float neighbours[6]) {
  const auto &props = mat();
  float cp = props.specific_heat;
  float k = props.thermal_conductivity;
  float rho = props.density_kgm3;

  // Time step (tunable)
  constexpr float time_step = 10.0f; // seconds
  const float L2 = WIDTH * WIDTH;    // face area = L²

  // Factor from the heat equation discretisation:
  // ΔT = k * A * Δt * ΔT_neighbour / (ρ * L² * cp * L)   (L cancels with A/L²)
  // → factor = k * Δt / (ρ * L² * cp)
  float factor = (k * time_step) / (rho * L2 * cp);

  // Conduction from six neighbours
  for (int i = 0; i < 6; ++i) {
    state.temperature += (neighbours[i] - state.temperature) * factor;
  }

  // Simplified ambient exchange (air convection / radiation)
  constexpr float ambient_factor = 0.01f;
  state.temperature += (ambient - state.temperature) * ambient_factor;
}

void Tile::updateLiquidVolume(float rainfall, float drainage,
                              float evaporation) {
  // Convert depths (m) to volume fractions
  float volume_gain = rainfall / HEIGHT;
  float volume_loss = (drainage + evaporation) / HEIGHT;
  float porosity = effective_porosity();

  state.liquid_volume += volume_gain;
  state.liquid_volume -= volume_loss;
  state.liquid_volume = std::clamp(state.liquid_volume, 0.0f, porosity);
}

void Tile::applyTrample(float weight, float area) {
  // Pressure in Pa (force = weight * g)
  float force = weight * 9.81f;
  float pressure = force / area; // Pa

  // Normalise pressure to MPa for meaningful scaling
  float pressure_MPa = pressure * 1.0e-6f;
  float resistance = mat().compaction_resistance;

  // Compaction increase is inversely proportional to resistance
  state.compaction += pressure_MPa * (1.0f - resistance) * 0.05f;
  state.compaction = std::clamp(state.compaction, 0.0f, 1.0f);
}