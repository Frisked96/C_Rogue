#include "entity_properties.hpp"
#include <unordered_map>

const EntityProperties& get_entity_properties(EntityType type) {
    static const std::unordered_map<EntityType, EntityProperties> catalogue = {
        {EntityType::PLAYER, 
         {"Player", '@', 15, 1.8f, 80.0f, 2500.0f, 2.5f, 0.01f, 0.005f, 1.0f, 10000.0f, 1.0f, 1.0f, 20}},
        {EntityType::NPC_HUMANOID, 
         {"Humanoid", 'H', 7, 1.75f, 75.0f, 2200.0f, 2.2f, 0.01f, 0.005f, 0.9f, 8000.0f, 0.9f, 0.9f, 15}},
        {EntityType::ANIMAL_RABBIT, 
         {"Rabbit", 'r', 15, 0.3f, 2.0f, 200.0f, 0.5f, 0.05f, 0.02f, 0.2f, 500.0f, 0.2f, 0.2f, 10}},
        {EntityType::ANIMAL_WOLF, 
         {"Wolf", 'w', 8, 0.8f, 40.0f, 1500.0f, 2.0f, 0.02f, 0.01f, 0.8f, 5000.0f, 0.8f, 0.8f, 18}}
    };

    auto it = catalogue.find(type);
    if (it != catalogue.end()) return it->second;
    return catalogue.at(EntityType::PLAYER);
}
