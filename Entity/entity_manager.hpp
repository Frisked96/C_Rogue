#pragma once

#include "spatial_system.hpp"
#include <algorithm>
#include <functional>
#include <memory>
#include <unordered_map>
#include <unordered_set>
#include <vector>

// Helper to build signatures from component types
template <typename... Components> struct SignatureBuilder {
  static Signature build() {
    Signature s;
    (void)std::initializer_list<int>{
        (s.set(BaseComponent<Components>::getComponentTypeId()), 0)...};
    return s;
  }
};

class EntityManager : public IEntityListener {
private:
  // Main storage
  std::unordered_map<int, std::unique_ptr<Entity>> entities;

  // Entity pool
  std::vector<std::unique_ptr<Entity>> pool;

  // Spatial Partitioning System
  SpatialGrid spatialGrid;

public:
  EntityManager();
  ~EntityManager();

  // Factory methods
  Entity *createEntity();

  template <typename T, typename... Args> T *createEntity(Args &&...args) {
    // T must inherit Entity
    auto uptr = std::make_unique<T>(std::forward<Args>(args)...);
    T *ptr = uptr.get();
    addEntity(std::move(uptr));
    return ptr;
  }

  void destroyEntity(int id);
  Entity *getEntity(int id);

  SpatialGrid &getSpatialGrid() { return spatialGrid; }
  const SpatialGrid &getSpatialGrid() const { return spatialGrid; }

  // Query
  // Returns all entities that possess at least the specified components.
  template <typename... Components> std::vector<Entity *> getEntitiesWith() {
    Signature mask = SignatureBuilder<Components...>::build();
    return getEntitiesMatching(mask);
  }

  std::vector<Entity *> getEntitiesMatching(Signature mask);

  // IEntityListener implementation
  void onEntitySignatureChanged(Entity *entity,
                                Signature newSignature) override;
  void onEntityMoved(Entity *entity, int oldX, int oldY, int newX,
                     int newY) override;

private:
  void addEntity(std::unique_ptr<Entity> entity);
};