#include "spatial_system.hpp"
#include "anatomy_components.hpp"
#include "anatomy_system.hpp"
#include "components.hpp"
#include <algorithm>
#include <cmath>

// Update entity in grid
void SpatialGrid::updateEntity(Entity *entity, int oldX, int oldY, int newX,
                               int newY) {
  if (oldX == newX && oldY == newY) {
    auto &vec = grid[getGridKey(newX, newY)];
    if (std::find(vec.begin(), vec.end(), entity) == vec.end()) {
      vec.push_back(entity);
    }
    return;
  }

  removeEntity(entity, oldX, oldY);
  grid[getGridKey(newX, newY)].push_back(entity);
}

void SpatialGrid::removeEntity(Entity *entity, int x, int y) {
  auto it = grid.find(getGridKey(x, y));
  if (it != grid.end()) {
    auto &vec = it->second;
    vec.erase(std::remove(vec.begin(), vec.end(), entity), vec.end());
    if (vec.empty()) {
      grid.erase(it);
    }
  }
}

std::vector<Entity *> SpatialGrid::getEntitiesAt(int x, int y) const {
  auto it = grid.find(getGridKey(x, y));
  if (it != grid.end()) {
    return it->second;
  }
  return {};
}

std::vector<Entity *> SpatialGrid::getEntitiesInRadius(int x, int y,
                                                       float radius) const {
  std::vector<Entity *> results;
  int r = static_cast<int>(std::ceil(radius));

  for (int dx = -r; dx <= r; ++dx) {
    for (int dy = -r; dy <= r; ++dy) {
      if (dx * dx + dy * dy <= radius * radius) {
        auto entities = getEntitiesAt(x + dx, y + dy);
        results.insert(results.end(), entities.begin(), entities.end());
      }
    }
  }
  return results;
}

bool SpatialGrid::areAdjacent(int x1, int y1, int x2, int y2) const {
  return std::abs(x1 - x2) <= 1 && std::abs(y1 - y2) <= 1;
}

std::vector<SpatialGrid::RayHit> SpatialGrid::raycast(int x1, int y1, int x2,
                                                      int y2) const {
  std::vector<RayHit> hits;

  int dx = std::abs(x2 - x1);
  int dy = std::abs(y2 - y1);
  int x = x1;
  int y = y1;
  int n = 1 + dx + dy;
  int x_inc = (x2 > x1) ? 1 : -1;
  int y_inc = (y2 > y1) ? 1 : -1;
  int error = dx - dy;
  dx *= 2;
  dy *= 2;

  for (; n > 0; --n) {
    float current_hit_offsetX = 0.0f;
    float current_hit_offsetY = 0.0f;

    if (x_inc != 0) {
      current_hit_offsetX = -0.5f * x_inc;
    }
    if (y_inc != 0) {
      current_hit_offsetY = -0.5f * y_inc;
    }

    auto entities = getEntitiesAt(x, y);
    for (auto *entity : entities) {
      BodyPart *part = determineHitLocation(entity, current_hit_offsetX,
                                            current_hit_offsetY);
      if (part) {
        hits.push_back({entity, part, x, y});
      }
    }

    if (error > 0) {
      x += x_inc;
      error -= dy;
    } else if (error < 0) {
      y += y_inc;
      error += dx;
    } else {
      x += x_inc;
      y += y_inc;
      error += dx - dy;
      n--;
    }
  }
  return hits;
}

BodyPart *SpatialGrid::determineHitLocation(Entity *target, float hitOffsetX,
                                            float hitOffsetY) const {
  if (!target->hasAnatomy())
    return nullptr;
  auto *anatomy = target->getAnatomy();
  return AnatomySystem::determineHitLocation(anatomy, hitOffsetX, hitOffsetY);
}
