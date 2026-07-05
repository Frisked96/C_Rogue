#pragma once
#include "../spatial_grid.hpp"
#include "entity.hpp"
#include <deque>
#include <unordered_map>
#include <vector>

class Game_map;

class EntityManager {
private:
  struct Slot {
    Entity entity;
    uint32_t generation;
    bool active;
  };

  std::vector<Slot> slots;
  std::deque<uint32_t> free_slots;
  SpatialGrid<EntityID> spatial_grid;

  EntityID make_id(uint32_t index, uint32_t generation) const {
    return (generation << 16) | (index & 0xFFFF);
  }
  uint32_t get_index(EntityID id) const { return id & 0xFFFF; }
  uint32_t get_generation(EntityID id) const { return id >> 16; }

public:
  EntityManager();

  EntityID spawn(EntityType type, int x, int y, int z);
  void kill(EntityID id);

  Entity *get(EntityID id);
  const Entity *get(EntityID id) const;

  void update(float dt);

  // For renderer/iteration
  std::vector<Entity *> get_all_active();

  SpatialGrid<EntityID> &get_spatial_grid() { return spatial_grid; }
  const SpatialGrid<EntityID> &get_spatial_grid() const { return spatial_grid; }

  // Movement check — combines spatial grid + map solidity
  bool is_blocked(int x, int y, int z, const Game_map &map) const;
};
