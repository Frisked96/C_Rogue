#include "system_manager.hpp"
#include "components.hpp"
#include <vector>

SystemManager::SystemManager() {}

void SystemManager::update(EntityManager& em) {
    // Hybrid Update: Only process entities that are "active"
    // Active entities are those that have suffered damage, stress, or are players.
    // We iterate over the active set.

    std::vector<int> toRemove;

    for (int entityId : active_anatomy_entities) {
        Entity* entity = em.getEntity(entityId);
        if (!entity) {
            toRemove.push_back(entityId); // Should have been caught by onEntityDestroyed
            continue;
        }

        // 1. Anatomy logic (vitals check, limb status)
        anatomy.processEntity(entity);
        
        // 2. Physiology logic (bleeding, metabolism, oxygen)
        // Requires HealthComponent to be alive
        bool stillActive = false;
        if (entity->hasComponent<HealthComponent>()) {
            auto* health = entity->getComponent<HealthComponent>();
            if (health->is_alive) {
                physiology.processEntity(entity, entity->getAnatomy(), health);

                // Check if we can deactivate this entity (stabilized)
                auto* anat = entity->getAnatomy();
                if (anat) {
                    stillActive = physiology.shouldKeepActive(anat, health);
                }
            }
        }

        if (!stillActive) {
            toRemove.push_back(entityId);
        }
    }

    for (int id : toRemove) {
        active_anatomy_entities.erase(id);
    }
}

AttackResult SystemManager::resolveAttack(Entity* attacker, Entity* defender, const DamageInfo& info) {
    AttackResult res = damageResolution.resolveAttack(attacker, defender, info);

    if (res.hit) {
        markActive(defender->getId());
        if (attacker) markActive(attacker->getId()); // Attacker might get tired/stressed
    }

    return res;
}

bool SystemManager::inflictWound(Entity* target, const std::string& partName, int damage, int bleedSeverity) {
    bool result = anatomy.inflictWound(target, partName, damage, bleedSeverity);
    if (result) {
        markActive(target->getId());
    }
    return result;
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
    markInactive(entity->getId());
}

void SystemManager::markActive(int entityId) {
    active_anatomy_entities.insert(entityId);
}

void SystemManager::markInactive(int entityId) {
    active_anatomy_entities.erase(entityId);
}
