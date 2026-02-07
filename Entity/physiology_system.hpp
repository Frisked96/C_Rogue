#pragma once

#include "entity_manager.hpp"
#include "anatomy_components.hpp"

class PhysiologySystem {
public:
    void processEntity(Entity* entity, AnatomyComponent* anatomy, HealthComponent* health);

private:
    // Sub-processes
    void processCirculation(AnatomyComponent* anatomy, HealthComponent* health);
    void processRespiration(AnatomyComponent* anatomy);
    void processMetabolism(AnatomyComponent* anatomy, HealthComponent* health);
    void processPain(AnatomyComponent* anatomy);
    void processHealing(AnatomyComponent* anatomy);

    // Helpers
    float calculateOrganEfficiency(AnatomyComponent* anatomy, Organ::Type type);
};
