#include "spatial_system.hpp"
#include "anatomy_components.hpp"
#include "anatomy_system.hpp"
#include "components.hpp"
#include "game_map.hpp"
#include <algorithm>
#include <cmath>

// Update entity in grid
void SpatialGrid::updateEntity(Entity *entity, int oldX, int oldY, int oldZ, int newX,
                               int newY, int newZ) {
  int height = 1;
  if (entity->hasSpatialProfile()) {
    height = entity->getSpatialProfile()->height_voxels;
  }

  // Remove from old positions
  for (int h = 0; h < height; ++h) {
    removeEntity(entity, oldX, oldY, oldZ + h);
  }

  // Add to new positions
  for (int h = 0; h < height; ++h) {
    grid[getGridKey(newX, newY, newZ + h)].push_back(entity);
  }
}

void SpatialGrid::removeEntity(Entity *entity, int x, int y, int z) {
  auto it = grid.find(getGridKey(x, y, z));
  if (it != grid.end()) {
    auto &vec = it->second;
    vec.erase(std::remove(vec.begin(), vec.end(), entity), vec.end());
    if (vec.empty()) {
      grid.erase(it);
    }
  }
}

bool SpatialGrid::canMoveTo(Entity *entity, int x, int y, int z, const Game_map &map) const {
  if (!map.is_in_bounds(x, y, z)) return false;

  int height = 1;
  if (entity->hasSpatialProfile()) {
    height = entity->getSpatialProfile()->height_voxels;
  }

  // 1. Check for voxel/material collisions and entity collisions in all occupied tiles
  for (int h = 0; h < height; ++h) {
    int checkZ = z + h;
    if (!map.is_in_bounds(x, y, checkZ)) return false;

    // Check voxel solidity
    if (map.get_tile(x, y, checkZ).mat().is_solid) return false;

    // Check for blocking entities
    auto entities = getEntitiesAt(x, y, checkZ);
    for (auto *other : entities) {
      if (other != entity && other->blocksMovement()) return false;
    }
  }

  // 2. Support Check (Anti-Air-Climbing)
  // To stand at (x,y,z), the tile at (x,y,z-1) must be solid or have a supporting entity
  if (z > 0) {
    bool hasSupport = false;
    // Check floor solidity
    if (map.get_tile(x, y, z - 1).mat().is_solid) {
      hasSupport = true;
    } else {
      // Check if any entity below can support us
      auto entitiesBelow = getEntitiesAt(x, y, z - 1);
      for (auto *other : entitiesBelow) {
        if (other != entity && other->blocksMovement()) {
          hasSupport = true;
          break;
        }
      }
    }

    // Special case for flying/climbing could be added here
    if (!hasSupport) return false;
  } else if (z == 0) {
      // Bottom of the map is always supported (or could be void)
      return true;
  }

  return true;
}

std::vector<Entity *> SpatialGrid::getEntitiesAt(int x, int y, int z) const {
  auto it = grid.find(getGridKey(x, y, z));
  if (it != grid.end()) {
    return it->second;
  }
  return {};
}

std::vector<Entity *> SpatialGrid::getEntitiesInRadius(int x, int y, int z,
                                                       float radius) const {
  std::vector<Entity *> results;
  int r = static_cast<int>(std::ceil(radius));

  for (int dx = -r; dx <= r; ++dx) {
    for (int dy = -r; dy <= r; ++dy) {
      for (int dz = -r; dz <= r; ++dz) {
        if (dx * dx + dy * dy + dz * dz <= radius * radius) {
          auto entities = getEntitiesAt(x + dx, y + dy, z + dz);
          results.insert(results.end(), entities.begin(), entities.end());
        }
      }
    }
  }
  return results;
}

bool SpatialGrid::areAdjacent(int x1, int y1, int z1, int x2, int y2, int z2) const {
  return std::abs(x1 - x2) <= 1 && std::abs(y1 - y2) <= 1 && std::abs(z1 - z2) <= 1;
}

std::vector<SpatialGrid::RayHit> SpatialGrid::raycast(int x1, int y1, int z1, int x2,
                                                      int y2, int z2) const {
  std::vector<RayHit> hits;
  // TODO: Full 3D Bresenham or similar for raycasting
  // For now, let's keep it limited to the same Z if z1 == z2, or just very basic.
  // Actually, I'll just implement a very simple 3D raycast.
  
  int dx = std::abs(x2 - x1);
  int dy = std::abs(y2 - y1);
  int dz = std::abs(z2 - z1);
  int x = x1;
  int y = y1;
  int z = z1;
  int n = 1 + dx + dy + dz;
  int x_inc = (x2 > x1) ? 1 : -1;
  int y_inc = (y2 > y1) ? 1 : -1;
  int z_inc = (z2 > z1) ? 1 : -1;
  
  int error1 = dx - dy;
  int error2 = dx - dz;
  
  dx *= 2;
  dy *= 2;
  dz *= 2;

  for (; n > 0; --n) {
    auto entities = getEntitiesAt(x, y, z);
    for (auto *entity : entities) {
      // Basic hit location (needs 3D version of determineHitLocation)
      BodyPart *part = determineHitLocation(entity, 0.0f, 0.0f, 0.0f);
      if (part) {
        hits.push_back({entity, part, x, y, z});
      }
    }

    if (dx >= dy && dx >= dz) {
        if (error1 > 0) { y += y_inc; error1 -= dx; }
        if (error2 > 0) { z += z_inc; error2 -= dx; }
        x += x_inc;
        error1 += dy;
        error2 += dz;
    } else if (dy >= dx && dy >= dz) {
        // ... and so on. Let's not get too bogged down in perfect 3D Bresenham 
        // unless necessary. Simple version:
        if (x != x2) x += x_inc;
        if (y != y2) y += y_inc;
        if (z != z2) z += z_inc;
    } else {
        if (z != z2) z += z_inc;
        if (x != x2) x += x_inc;
        if (y != y2) y += y_inc;
    }
  }
  return hits;
}

BodyPart *SpatialGrid::determineHitLocation(Entity *target, float hitOffsetX,
                                            float hitOffsetY, float hitOffsetZ) const {
  if (!target->hasAnatomy())
    return nullptr;
  auto *anatomy = target->getAnatomy();
  // AnatomySystem::determineHitLocation currently only takes 2 offsets
  return AnatomySystem::determineHitLocation(anatomy, hitOffsetX, hitOffsetY);
}
