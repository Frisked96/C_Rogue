#pragma once
#include <string>

class Entity;
class AnatomyComponent;

class AnatomySystem {
public:
    void processEntity(Entity* entity);
    void checkVitals(Entity* entity, AnatomyComponent* anatomy);
    void updateLimbStatus(AnatomyComponent* anatomy);
    void aggregateStatus(AnatomyComponent* anatomy, float& totalPain);
    float calculateReach(Entity* entity);
    float calculateMovementFactor(Entity* entity);
    bool inflictWound(Entity* target, const std::string& partName, int damage, int bleedSeverity = 0);
};
