#include "entity_manager.hpp"
#include <typeinfo>

EntityManager::EntityManager() : externalListener(nullptr) {}

EntityManager::~EntityManager() {
  entities.clear();
  pool.clear();
}

Entity *EntityManager::createEntity() {
  // Check pool first
  if (!pool.empty()) {
    auto uptr = std::move(pool.back());
    pool.pop_back();
    Entity *ptr = uptr.get();
    ptr->reset(); // Reset state
    addEntity(std::move(uptr));
    return ptr;
  }

  // Create new
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

    // Notify listener for cleanup (SystemManager usually)
    if (externalListener) {
      externalListener->onEntityDestroyed(ptr);
    }

    // Reuse if it is a plain Entity
    if (typeid(*ptr) == typeid(Entity)) {
       pool.push_back(std::move(it->second));
    }

    entities.erase(it);
  }
}

Entity* EntityManager::getEntity(int id) {
    auto it = entities.find(id);
    if (it != entities.end()) {
        return it->second.get();
    }
    return nullptr;
}

std::vector<Entity *> EntityManager::getEntitiesMatching(Signature mask) {
    std::vector<Entity*> results;
    for (auto& pair : entities) {
        Entity* e = pair.second.get();
        if ((e->getSignature() & mask) == mask) {
            results.push_back(e);
        }
    }
    return results;
}

// IEntityListener Implementation

void EntityManager::onEntitySignatureChanged(Entity *entity,
                                             Signature newSignature) {
  if (externalListener) {
    externalListener->onEntitySignatureChanged(entity, newSignature);
  }
}

void EntityManager::onEntityMoved(Entity *entity, int oldX, int oldY, int newX,
                                 int newY) {
  if (externalListener) {
    externalListener->onEntityMoved(entity, oldX, oldY, newX, newY);
  }
}

void EntityManager::onEntityDestroyed(Entity *entity) {
  if (externalListener) {
    externalListener->onEntityDestroyed(entity);
  }
}
