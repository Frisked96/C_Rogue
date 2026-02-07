#pragma once

#include "entity_manager.hpp"
#include "anatomy_components.hpp"
#include <vector>

class AnatomySystem {
public:
    // Update all entities with anatomy
    // Handles bleeding, vital checks, functionality updates
    void update(EntityManager& entityManager);

    // Apply specific logic for limb reach calculations (e.g., can entity reach x,y?)
    // This is a helper for other systems (CombatSystem)
    float calculateReach(Entity* entity);

    // Calculate movement penalty based on leg status (0.0 to 1.0, where 1.0 is full movement)
    float calculateMovementFactor(Entity* entity);

    // Inflict a wound that might cause bleeding
    // Returns true if wound was applied
    bool inflictWound(Entity* target, const std::string& partName, int damage, int bleedSeverity = 0);

private:
    void processEntity(Entity* entity);
    void checkVitals(Entity* entity, AnatomyComponent* anatomy);
    void updateLimbStatus(AnatomyComponent* anatomy);

    void aggregateStatus(AnatomyComponent* anatomy, float& totalPain);
    void aggregatePartStatus(BodyPart* part, float& totalPain);

    // Recursive helpers
    float calculateLegEfficiency(const std::vector<std::unique_ptr<BodyPart>>& parts);
};
