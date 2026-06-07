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
    int z;
  };

  // Update entity position in grid (handles multi-tile occupancy)
  void updateEntity(Entity *entity, int oldX, int oldY, int oldZ, int newX, int newY, int newZ);

  // Remove entity from grid completely
  void removeEntity(Entity *entity, int x, int y, int z);

  // Queries
  std::vector<Entity *> getEntitiesAt(int x, int y, int z) const;
  std::vector<Entity *> getEntitiesInRadius(int x, int y, int z, float radius) const;

  // Check if a move is valid (collision detection)
  bool canMoveTo(Entity *entity, int x, int y, int z, const class Game_map &map) const;

  // Adjacency check
  bool areAdjacent(int x1, int y1, int z1, int x2, int y2, int z2) const;

  // Raycasting for targeting
  std::vector<RayHit> raycast(int x1, int y1, int z1, int x2, int y2, int z2) const;

  // Hit location determination
  BodyPart *determineHitLocation(Entity *target, float hitOffsetX,
                                 float hitOffsetY, float hitOffsetZ) const;

private:
  std::unordered_map<long long, std::vector<Entity *>> grid;

  static long long getGridKey(int x, int y, int z) {
    return ((static_cast<long long>(x + 524288) & 0xFFFFF) << 40) |
           ((static_cast<long long>(y + 524288) & 0xFFFFF) << 20) |
           (static_cast<long long>(z + 524288) & 0xFFFFF);
  }
};