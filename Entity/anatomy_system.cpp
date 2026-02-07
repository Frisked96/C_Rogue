#include "anatomy_system.hpp"
#include "components.hpp"
#include <cstdlib>

void AnatomySystem::update(EntityManager& entityManager) {
    auto entities = entityManager.getEntitiesWith<AnatomyComponent>();
    for (auto* entity : entities) {
        processEntity(entity);
    }
}

void AnatomySystem::processEntity(Entity* entity) {
    if (!entity->hasAnatomy()) return;
    
    auto* anatomy = entity->getAnatomy();
    
    // 1. Check Vital Organs (death condition)
    checkVitals(entity, anatomy);

    // 2. Update Functionality
    updateLimbStatus(anatomy);

    // 3. Aggregate Pain and arterial integrity
    float totalPain = 0.0f;
    aggregateStatus(anatomy, totalPain);
    anatomy->accumulated_pain = totalPain;
}

void AnatomySystem::aggregateStatus(AnatomyComponent* anatomy, float& totalPain) {
    for (auto& part : anatomy->body_parts) {
        aggregatePartStatus(part.get(), totalPain);
    }
}

void AnatomySystem::aggregatePartStatus(BodyPart* part, float& totalPain) {
    totalPain += part->pain_level;
    // We could also aggregate average arterial integrity if needed for physiology
    for (auto& internal : part->internal_parts) {
        aggregatePartStatus(internal.get(), totalPain);
    }
}

void AnatomySystem::checkVitals(Entity* entity, AnatomyComponent* anatomy) {
    if (!anatomy->isFunctional()) {
        // Kill entity
        if (entity->hasComponent<HealthComponent>()) {
            auto* health = entity->getComponent<HealthComponent>();
            if (health->is_alive) {
                health->is_alive = false;
                health->current_health = 0;
            }
        }
    }
}

void AnatomySystem::updateLimbStatus(AnatomyComponent* anatomy) {
    // Logic to update derived stats based on limb health
    // For example, if arm strength drops due to damage?
    // Currently BodyPart::takeDamage handles is_functional logic.
    // We could add complex logic here like: if parent limb is dead, child limb is useless.
    // But our structure is nested, so if parent is destroyed, we generally can't access children well logically
    // (though they exist in the vector).
    
    // Todo: Implement cascading failure if desired.
}

float AnatomySystem::calculateReach(Entity* entity) {
    if (!entity->hasAnatomy()) return 1.0f; // Default reach
    
    float maxReach = 0.0f;
    auto* anatomy = entity->getAnatomy();
    
    // Find functional arms
    for (auto& part : anatomy->body_parts) {
        // Assuming Arms are root parts or check recursively
        // Our Player setup puts Arms as root parts.
        auto* limb = dynamic_cast<Limb*>(part.get());
        if (limb && limb->limb_type == Limb::Type::ARM && limb->is_functional) {
             // Basic reach is 1.0 + potentially weapon length?
             // For now, return 1.5 for a working arm, 1.0 otherwise
             maxReach = std::max(maxReach, 1.5f);
        }
    }
    
    return maxReach > 0.0f ? maxReach : 1.0f; // Minimal reach if no arms? (Bite?)
}

// Helper to find legs recursively
void collectLegs(BodyPart* part, std::vector<Limb*>& legs) {
    auto* limb = dynamic_cast<Limb*>(part);
    if (limb && limb->limb_type == Limb::Type::LEG) {
        legs.push_back(limb);
    }
    for (auto& internal : part->internal_parts) {
        collectLegs(internal.get(), legs);
    }
}

float AnatomySystem::calculateMovementFactor(Entity* entity) {
    if (!entity->hasAnatomy()) return 1.0f;

    auto* anatomy = entity->getAnatomy();
    std::vector<Limb*> legs;
    
    for (auto& part : anatomy->body_parts) {
        collectLegs(part.get(), legs);
    }

    if (legs.empty()) return 1.0f; // No legs defined, assume flying or snake? :P

    int functionalLegs = 0;
    for (auto* leg : legs) {
        if (leg->is_functional) functionalLegs++;
    }

    if (functionalLegs == 0) return 0.0f; // Cant move
    if (functionalLegs < legs.size()) return 0.5f; // Hobble

    return 1.0f;
}

bool AnatomySystem::inflictWound(Entity* target, const std::string& partName, int damage, int bleedSeverity) {
    if (!target->hasAnatomy()) {
        // Fallback to simple health damage if no anatomy
        if (target->hasComponent<HealthComponent>()) {
            target->getComponent<HealthComponent>()->takeDamage(damage);
            return true;
        }
        return false;
    }

    auto* anatomy = target->getAnatomy();
    BodyPart* part = anatomy->getBodyPart(partName); // Using existing helper

    if (part) {
        part->takeDamage(damage);
        part->bleeding_intensity += bleedSeverity;
        return true;
    }
    
    return false;
}
