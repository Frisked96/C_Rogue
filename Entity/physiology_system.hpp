#pragma once
#include "anatomy_components.hpp"
#include "components.hpp"

class Entity;

class PhysiologySystem {
public:
    void processEntity(Entity* entity, AnatomyComponent* anatomy, HealthComponent* health);

private:
    void processCirculation(AnatomyComponent* anatomy, HealthComponent* health);
    void processRespiration(AnatomyComponent* anatomy);
    void processMetabolism(AnatomyComponent* anatomy, HealthComponent* health);
    void processPain(AnatomyComponent* anatomy);
    void processStress(AnatomyComponent* anatomy);
    void processHealing(AnatomyComponent* anatomy);

    float calculateOrganEfficiency(AnatomyComponent* anatomy, OrganType type);
};
