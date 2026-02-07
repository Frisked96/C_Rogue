#include "system_manager.hpp"
#include "components.hpp"

SystemManager::SystemManager() {}

void SystemManager::update(EntityManager& em) {
    // Single pass over entities with Anatomy to process all biological systems
    auto entities = em.getEntitiesWith<AnatomyComponent>();
    for (auto* entity : entities) {
        // 1. Anatomy logic (vitals check, limb status)
        anatomy.processEntity(entity);
        
        // 2. Physiology logic (bleeding, metabolism, oxygen)
        // Requires HealthComponent to be alive
        if (entity->hasComponent<HealthComponent>()) {
            auto* health = entity->getComponent<HealthComponent>();
            if (health->is_alive) {
                physiology.processEntity(entity, entity->getAnatomy(), health);
            }
        }
    }
}

AttackResult SystemManager::resolveAttack(Entity* attacker, Entity* defender, const DamageInfo& info) {
    return damageResolution.resolveAttack(attacker, defender, info);
}

bool SystemManager::inflictWound(Entity* target, const std::string& partName, int damage, int bleedSeverity) {
    return anatomy.inflictWound(target, partName, damage, bleedSeverity);
}

void SystemManager::onEntitySignatureChanged(Entity* entity, Signature newSignature) {
    static size_t posBit = BaseComponent<PositionComponent>::getComponentTypeId();
    
    if (newSignature.test(posBit)) {
        if (auto* pos = entity->getComponent<PositionComponent>()) {
            spatialGrid.updateEntity(entity, pos->x, pos->y, pos->x, pos->y);
        }
    }
}

void SystemManager::onEntityMoved(Entity* entity, int oldX, int oldY, int newX, int newY) {
    spatialGrid.updateEntity(entity, oldX, oldY, newX, newY);
}

void SystemManager::onEntityDestroyed(Entity* entity) {
    if (auto* pos = entity->getComponent<PositionComponent>()) {
        spatialGrid.removeEntity(entity, pos->x, pos->y);
    }
}
