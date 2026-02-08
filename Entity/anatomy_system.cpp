#include "anatomy_system.hpp"
#include "anatomy_components.hpp"
#include "components.hpp"
#include "entity.hpp"
#include <cstdlib>

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
    for (const auto& part : anatomy->body_parts) {
        totalPain += part.pain_level;
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
    // In flat structure, we can iterate and check parent
    
    for (auto& part : anatomy->body_parts) {
        if (part.parent_index != -1) {
            // If parent is not functional, child is also not functional (e.g. severed arm -> hand useless)
            // Note: This requires parent to be processed before child or multiple passes if indices are unordered.
            // Assuming factory creates parents first (smaller index).
            if (!anatomy->body_parts[part.parent_index].is_functional) {
                part.is_functional = false;
            }
        }
    }
}

float AnatomySystem::calculateReach(Entity* entity) {
    if (!entity->hasAnatomy()) return 1.0f; // Default reach
    
    float maxReach = 0.0f;
    auto* anatomy = entity->getAnatomy();
    
    // Find functional arms
    for (const auto& part : anatomy->body_parts) {
        if (part.type == BodyPartType::LIMB && part.limb_type == LimbType::ARM && part.is_functional) {
             // Basic reach is 1.0 + potentially weapon length?
             // For now, return 1.5 for a working arm, 1.0 otherwise
             maxReach = std::max(maxReach, 1.5f);
        }
    }
    
    return maxReach > 0.0f ? maxReach : 1.0f; // Minimal reach if no arms? (Bite?)
}

float AnatomySystem::calculateMovementFactor(Entity* entity) {
    if (!entity->hasAnatomy()) return 1.0f;

    auto* anatomy = entity->getAnatomy();
    int totalLegs = 0;
    int functionalLegs = 0;
    
    for (const auto& part : anatomy->body_parts) {
        if (part.type == BodyPartType::LIMB && part.limb_type == LimbType::LEG) {
            totalLegs++;
            if (part.is_functional) {
                functionalLegs++;
            }
        }
    }

    if (totalLegs == 0) return 1.0f; // No legs defined, assume flying or snake? :P

    if (functionalLegs == 0) return 0.0f; // Cant move
    if (functionalLegs < totalLegs) return 0.5f; // Hobble

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
    BodyPart* part = anatomy->getBodyPart(partName);

    if (part) {
        part->takeDamage(damage);
        part->bleeding_intensity += bleedSeverity;
        return true;
    }
    
    return false;
}

// Recursive helper for AABB containment using indices
static BodyPart* checkHitRecursive(AnatomyComponent& anatomy, int partIndex, float localX, float localY) {
    if (partIndex < 0 || partIndex >= (int)anatomy.body_parts.size()) return nullptr;

    BodyPart& part = anatomy.body_parts[partIndex];

    float halfW = part.width / 2.0f;
    float halfH = part.height / 2.0f;

    // Check hit on part's bounding box
    if (localX < -halfW || localX > halfW || localY < -halfH || localY > halfH) {
        return nullptr;
    }

    // Check children (internal parts)
    for (int childIdx : part.children_indices) {
        if (childIdx < 0 || childIdx >= (int)anatomy.body_parts.size()) continue;

        BodyPart& child = anatomy.body_parts[childIdx];

        // Transform point to child's local space
        float childLocalX = localX - child.relative_x;
        float childLocalY = localY - child.relative_y;

        BodyPart* hitChild = checkHitRecursive(anatomy, childIdx, childLocalX, childLocalY);
        if (hitChild) {
            return hitChild;
        }
    }

    // No internal part hit, so we hit this part
    return &part;
}

BodyPart* AnatomySystem::determineHitLocation(AnatomyComponent* anatomy, float localX, float localY) {
    if (!anatomy) return nullptr;

    // Check against all root body parts
    for (int i = 0; i < (int)anatomy->body_parts.size(); ++i) {
        BodyPart& part = anatomy->body_parts[i];

        if (part.parent_index == -1) { // Root part
            float partLocalX = localX - part.relative_x;
            float partLocalY = localY - part.relative_y;

            BodyPart* hit = checkHitRecursive(*anatomy, i, partLocalX, partLocalY);
            if (hit) {
                return hit;
            }
        }
    }

    return nullptr;
}
