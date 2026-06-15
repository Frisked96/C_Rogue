#include "spatial_grid.hpp"
#include "../game_map.hpp"

bool SpatialGrid::is_blocked(int x, int y, int z, const Game_map &map) const {
  if (!map.is_in_bounds(x, y, z))
    return true;

  // Check map solidity
  if (map.get_tile(x, y, z).mat().is_solid)
    return true;

  // Check for other entities
  const auto &entities = get_at(x, y, z);
  if (!entities.empty())
    return true; // Simple: any entity blocks

  return false;
}
