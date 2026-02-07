#pragma once
#include "entity.hpp"
#include <unordered_map>
#include <vector>

class BodyPart;

class SpatialGrid {
public:
  struct RayHit {
    Entity *entity;
    BodyPart *part;
    int x;
    int y;
  };

  // Update entity position in grid
  void updateEntity(Entity *entity, int oldX, int oldY, int newX, int newY);

  // Remove entity from grid completely
  void removeEntity(Entity *entity, int x, int y);

  // Queries
  std::vector<Entity *> getEntitiesAt(int x, int y) const;
  std::vector<Entity *> getEntitiesInRadius(int x, int y, float radius) const;

  // Adjacency check
  bool areAdjacent(int x1, int y1, int x2, int y2) const;

  // Raycasting for targeting
  // Returns all entities hit along the line, sorted by distance
  std::vector<RayHit> raycast(int x1, int y1, int x2, int y2) const;

  // Hit location determination
  // hitOffsetX/Y are relative to entity center [-0.5, 0.5]
  BodyPart *determineHitLocation(Entity *target, float hitOffsetX,
                                 float hitOffsetY) const;

private:
  std::unordered_map<long long, std::vector<Entity *>> grid;

  static long long getGridKey(int x, int y) {
    return (static_cast<long long>(x) << 32) | (static_cast<unsigned int>(y));
  }
};
