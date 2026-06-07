#include "system_manager.hpp"
#include "anatomy_components.hpp"
#include "anatomy_system.hpp"
#include "components.hpp"
#include "physiology_system.hpp"
#include <iostream>

SystemManager::SystemManager() {}

void SystemManager::update(EntityManager &em) {
  auto entities = em.getAllEntities();
  for (auto *entity : entities) {
    if (entity->hasComponent<AnatomyComponent>() && entity->hasComponent<HealthComponent>()) {
      auto *anatomyComp = entity->getComponent<AnatomyComponent>();
      auto *healthComp = entity->getComponent<HealthComponent>();
      physiology.processEntity(entity, anatomyComp, healthComp);
    }
  }

  // 2. Anatomy Update (Regeneration, etc - only for active entities)
  for (int id : active_anatomy_entities) {
    if (auto *entity = em.getEntity(id)) {
      anatomy.processEntity(entity);
    }
  }
}

AttackResult SystemManager::resolveAttack(Entity *attacker, Entity *defender,
                                          const DamageInfo &info) {
  return damageResolution.resolveAttack(attacker, defender, info);
}

bool SystemManager::inflictWound(Entity *target, const std::string &partName,
                                 int damage, int bleedSeverity) {
  if (!target->hasAnatomy())
    return false;
  auto *anatomyComp = target->getAnatomy();
  int idx = anatomyComp->getBodyPartIndex(partName);
  if (idx == -1)
    return false;

  anatomyComp->body_parts[idx].current_hitpoints -= damage;
  if (bleedSeverity > 0) {
    anatomyComp->body_parts[idx].bleeding_intensity += bleedSeverity;
  }
  return true;
}

void SystemManager::onEntitySignatureChanged(Entity *entity,
                                              Signature newSignature) {
  static size_t anatomyBit =
      BaseComponent<AnatomyComponent>::getComponentTypeId();
  static size_t posBit = BaseComponent<PositionComponent>::getComponentTypeId();

  if (newSignature.test(anatomyBit)) {
    markActive(entity->getId());
  } else {
    markInactive(entity->getId());
  }

  if (newSignature.test(posBit)) {
    if (auto *pos = entity->getComponent<PositionComponent>()) {
      spatialGrid.updateEntity(entity, pos->x, pos->y, pos->z, pos->x, pos->y, pos->z);
    }
  }
}

void SystemManager::onEntityMoved(Entity *entity, int oldX, int oldY, int oldZ, int newX,
                                  int newY, int newZ) {
  spatialGrid.updateEntity(entity, oldX, oldY, oldZ, newX, newY, newZ);
}

void SystemManager::onEntityDestroyed(Entity *entity) {
  if (auto *pos = entity->getComponent<PositionComponent>()) {
    int height = 1;
    if (entity->hasSpatialProfile()) {
      height = entity->getSpatialProfile()->height_voxels;
    }
    for (int h = 0; h < height; ++h) {
      spatialGrid.removeEntity(entity, pos->x, pos->y, pos->z + h);
    }
  }
  markInactive(entity->getId());
}

void SystemManager::markActive(int entityId) {
  active_anatomy_entities.insert(entityId);
}

void SystemManager::markInactive(int entityId) {
  active_anatomy_entities.erase(entityId);
}
