#include "tile.hpp"
#include <algorithm>

namespace Tiles {
const Tile Wall{MaterialType::STONE_GRANITE};
const Tile Floor{MaterialType::SOIL_LOAM};
} // namespace Tiles

void Tile::updateTemperature(float ambient, float neighbours[4]) {
  // Simple conduction model
  float conductivity = mat().thermal_conductivity;
  float total_neighbor_temp = 0;
  for (int i = 0; i < 4; ++i)
    total_neighbor_temp += neighbours[i];

  float avg_neighbor_temp = total_neighbor_temp / 4.0f;

  // Change based on neighbors and ambient
  state.temperature += (avg_neighbor_temp - state.temperature) * conductivity * 0.1f;
  state.temperature += (ambient - state.temperature) * 0.01f;
}

void Tile::updateMoisture(float rainfall, float drainage, float evaporation) {
  float porosity = effective_porosity();
  state.moisture += rainfall;
  state.moisture -= (drainage + evaporation);
  state.moisture = std::clamp(state.moisture, 0.0f, porosity);
}

void Tile::applyTrample(float weight, float area) {
  float pressure = weight / area;
  float resistance = mat().compaction_resistance;
  // Increase compaction based on pressure and resistance
  state.compaction += (pressure / 1000.0f) * (1.0f - resistance) * 0.1f;
  state.compaction = std::clamp(state.compaction, 0.0f, 1.0f);
}
