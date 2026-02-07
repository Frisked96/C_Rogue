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

Entity *EntityManager::getEntity(int id) {
  auto it = entities.find(id);
  if (it != entities.end()) {
    return it->second.get();
  }
  return nullptr;
}

std::vector<Entity *> EntityManager::getEntitiesMatching(Signature mask) {
  std::vector<Entity *> results;
  // Heuristic reserve
  if (!entities.empty()) {
    results.reserve(entities.size() / 4);
  }

  for (auto &pair : entities) {
    Entity *e = pair.second.get();
    if ((e->getSignature() & mask) == mask) {
      results.push_back(e);
    }
  }
  return results;
}

void EntityManager::onEntitySignatureChanged(Entity *entity,
                                             Signature newSignature) {
  // This allows for future optimizations, such as updating cached lists
  // or notifying specific systems.
}
