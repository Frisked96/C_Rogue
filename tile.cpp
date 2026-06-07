#include "tile.hpp"
#include <algorithm>
#include <cmath>

namespace Tiles {
const Tile Wall{MaterialType::STONE_GRANITE};
const Tile Floor{MaterialType::SOIL_LOAM};
} // namespace Tiles

void Tile::updateTemperature(float ambient, float neighbours[6]) {
  const auto &props = mat();
  float cp = props.specific_heat;
  float k = props.thermal_conductivity;

  // Assume each turn is some number of seconds (e.g., 60s)
  // For stability and realism, we use the physical formula:
  // delta_T = (k * Area * delta_temp * time) / (mass * specific_heat * distance)
  // Since Area = L*L, distance = L, mass = rho * L*L*L
  // delta_T = (k * delta_temp * time) / (rho * L*L * specific_heat)
  
  float time_step = 10.0f; // 10 seconds per update for now
  float L2 = WIDTH * WIDTH; // Assuming cubic L=WIDTH=HEIGHT=DEPTH
  
  // Thermal inertia factor
  float factor = (k * time_step) / (props.density_kgm3 * L2 * cp);
  
  // Neighbor conduction (6 faces)
  for (int i = 0; i < 6; ++i) {
    state.temperature += (neighbours[i] - state.temperature) * factor;
  }

  // Ambient exchange (simplified, assuming some surface exposure or air mix)
  // This could be more complex based on how much surface is exposed to "sky"
  float ambient_factor = 0.01f; // Simplified air exchange rate
  state.temperature += (ambient - state.temperature) * ambient_factor;
}

void Tile::updateMoisture(float rainfall, float drainage, float evaporation) {
  float porosity = effective_porosity();
  
  // rainfall is typically in meters (depth)
  // Convert to volume fraction: fraction = depth / height
  float moisture_gain = rainfall / HEIGHT;
  float moisture_loss = (drainage + evaporation) / HEIGHT;

  state.moisture += moisture_gain;
  state.moisture -= moisture_loss;
  state.moisture = std::clamp(state.moisture, 0.0f, porosity);
}

void Tile::applyTrample(float weight, float area) {
  // Pressure = force / area. Force = weight * g (approx weight in kg * 9.8)
  float force = weight * 9.81f;
  float pressure = force / area; // area in m^2
  
  float resistance = mat().compaction_resistance;
  // Increase compaction based on pressure vs resistance
  // 1 MPa is a lot of pressure, let's normalize
  float pressure_mpa = pressure / 1000000.0f; 
  
  state.compaction += (pressure_mpa) * (1.0f - resistance) * 0.05f;
  state.compaction = std::clamp(state.compaction, 0.0f, 1.0f);
}
