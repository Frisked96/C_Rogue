#include "entity_manager.hpp"
#include <typeinfo>

EntityManager::EntityManager() {}

EntityManager::~EntityManager() {
  entities.clear();
  pool.clear();
}

Entity *EntityManager::createEntity() {
  if (!pool.empty()) {
    auto uptr = std::move(pool.back());
    pool.pop_back();
    Entity *ptr = uptr.get();
    // Reset state and generate new ID
    ptr->reset();
    addEntity(std::move(uptr));
    return ptr;
  }

  auto uptr = std::make_unique<Entity>();
  Entity *ptr = uptr.get();
  addEntity(std::move(uptr));
  return ptr;
}

void EntityManager::addEntity(std::unique_ptr<Entity> entity) {
  if (entity) {
    entity->setListener(this);
    entities[entity->getId()] = std::move(entity);
  }
}

void EntityManager::destroyEntity(int id) {
  auto it = entities.find(id);
  if (it != entities.end()) {
    Entity *ptr = it->second.get();

    // Remove from spatial grid if it has a position
    if (auto *pos = ptr->getComponent<PositionComponent>()) {
      spatialGrid.removeEntity(ptr, pos->x, pos->y);
    }

    // Check if it's a base Entity to pool it
    // Only pool generic entities to avoid object slicing/polymorphic confusion
    if (typeid(*ptr) == typeid(Entity)) {
      // Move to pool
      pool.push_back(std::move(it->second));
    }
    // If it's a Player or other subclass, it will be destroyed when erased

    entities.erase(it);
  }
}

// ... (getEntity and getEntitiesMatching)

void EntityManager::onEntitySignatureChanged(Entity *entity,
                                             Signature newSignature) {
  // If entity gained PositionComponent, add it to spatial grid
  // If it lost it, remove it
  static size_t posBit = BaseComponent<PositionComponent>::getComponentTypeId();
  
  if (newSignature.test(posBit)) {
    if (auto *pos = entity->getComponent<PositionComponent>()) {
      spatialGrid.updateEntity(entity, pos->x, pos->y, pos->x, pos->y);
    }
  } else {
    // We don't know the old position easily here if it just lost the component
    // But SpatialGrid can be updated to handle this if we store old position somewhere
    // For now, let's assume entities don't often lose PositionComponent without being destroyed
  }
}

void EntityManager::onEntityMoved(Entity *entity, int oldX, int oldY, int newX,
                                 int newY) {
  spatialGrid.updateEntity(entity, oldX, oldY, newX, newY);
}
