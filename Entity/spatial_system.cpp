#include "spatial_system.hpp"
#include "anatomy_components.hpp"
#include "components.hpp"
#include <algorithm>
#include <cmath>

void SpatialGrid::updateEntity(Entity *entity, int oldX, int oldY, int newX, int newY) {
  if (oldX == newX && oldY == newY) {
    // Only add if not already there, but usually this is called on move
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

std::vector<Entity *> SpatialGrid::getEntitiesInRadius(int x, int y, float radius) const {
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

std::vector<SpatialGrid::RayHit> SpatialGrid::raycast(int x1, int y1, int x2, int y2) const {
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
      // If this is not the starting point, calculate a more meaningful hit offset
      // based on the direction the ray is coming from into the current cell (x, y).
      float current_hit_offsetX = 0.0f;
      float current_hit_offsetY = 0.0f;
  
      // These approximations place the hit point on the edge of the tile
      // that the ray just entered from, relative to the tile's center (0.0, 0.0).
      // e.g., if x_inc is 1, ray is moving right, so it entered (x,y) from its left side.
      // The relative hit X position is then roughly -0.5.
      if (x_inc != 0) {
          current_hit_offsetX = -0.5f * x_inc; 
      }
      if (y_inc != 0) {
          current_hit_offsetY = -0.5f * y_inc;
      }
      // If both x_inc and y_inc are non-zero (diagonal), the above gives
      // e.g., (-0.5, -0.5) for top-left entry, which is reasonable for a corner hit.
  
      auto entities = getEntitiesAt(x, y);
      for (auto *entity : entities) {
        BodyPart *part = determineHitLocation(entity, current_hit_offsetX, current_hit_offsetY);
        hits.push_back({entity, part, x, y});
      }
  
      if (error > 0) {
        x += x_inc;
        error -= dy;
      } else if (error < 0) {
        y += y_inc;
        error += dx;
      } else { // error == 0, diagonal move
          x += x_inc;
          y += y_inc;
          error += dx - dy;
          n--; // moved twice
      }
    }
  return hits;
}

// Recursive helper for AABB containment
BodyPart* checkHitRecursive(BodyPart* part, float localX, float localY) {
    // Check if point is inside this part's rectangle (centered at 0,0 local space)
    float halfW = part->width / 2.0f;
    float halfH = part->height / 2.0f;

    if (localX < -halfW || localX > halfW || localY < -halfH || localY > halfH) {
        return nullptr;
    }

    // Point is inside this part. Now check if it hits any internal part.
    // We check children in reverse order or just order? Order doesn't matter much unless overlapping.
    for (const auto& child : part->internal_parts) {
        // Transform point to child's local space
        // Child pos is relative to this part's center
        float childLocalX = localX - child->relative_x;
        float childLocalY = localY - child->relative_y;

        BodyPart* hitChild = checkHitRecursive(child.get(), childLocalX, childLocalY);
        if (hitChild) {
            return hitChild; // Hit an internal organ!
        }
    }

    // If no child was hit, we hit this part itself (the "meat/container")
    return part;
}

BodyPart *SpatialGrid::determineHitLocation(Entity *target, float hitOffsetX, float hitOffsetY) const {
  if (!target->hasAnatomy())
    return nullptr;

  auto *anatomy = target->getAnatomy();
  
  // Check against all root body parts
  for (auto &part : anatomy->body_parts) {
      // Transform hit to part's local space (relative to entity center)
      float partLocalX = hitOffsetX - part->relative_x;
      float partLocalY = hitOffsetY - part->relative_y;

      BodyPart* hit = checkHitRecursive(part.get(), partLocalX, partLocalY);
      if (hit) {
          return hit;
      }
  }

  return nullptr;
}