#include "entity_factory.hpp"
#include "components.hpp"
#include "anatomy_components.hpp"
#include <memory>

EntityFactory::EntityFactory(EntityManager& em) : entityManager(em) {}

Entity* EntityFactory::createPlayer(int x, int y, const std::string& name, char glyph) {
    // Create a generic entity instead of a specific Player class
    Entity* entity = entityManager.createEntity();

    // Add components that define a player
    entity->addComponent<PositionComponent>(x, y);
    entity->addComponent<RenderComponent>(glyph);
    entity->addComponent<NameComponent>(name);
    entity->addComponent<BlockingComponent>();
    entity->addComponent<FactionComponent>(FactionComponent::Faction::PLAYER, false);
    
    // Add health component (standard for player)
    entity->addComponent<HealthComponent>(100);

    // Setup Anatomy
    auto& anatomy = entity->addComponent<AnatomyComponent>();

    // 1. Torso (Root Container) - Center of body
    auto torso = std::make_unique<BodyPart>("Torso", 50, true, false, 0.0f, 0.0f, 0.6f, 0.8f);

    // Add Heart inside Torso
    auto heart = std::make_unique<Organ>("Heart", Organ::Type::Heart, 10, -0.1f, -0.1f, 0.15f, 0.15f, true);
    torso->addInternalPart(std::move(heart));

    // Add Lungs inside Torso
    auto lungs = std::make_unique<Organ>("Lungs", Organ::Type::Lung, 15, 0.1f, -0.1f, 0.2f, 0.2f, true);
    torso->addInternalPart(std::move(lungs));

    anatomy.addBodyPart(std::move(torso));

    // 2. Head (Root Container)
    auto head = std::make_unique<Limb>("Head", Limb::Type::HEAD, 20, 0.0f, -0.6f, 0.3f, 0.3f, 1.0f, 1.0f, true);

    // Brain inside Head
    auto brain = std::make_unique<Organ>("Brain", Organ::Type::Brain, 5, 0.0f, 0.0f, 0.1f, 0.1f, true);
    head->addInternalPart(std::move(brain));

    anatomy.addBodyPart(std::move(head));

    // 3. Arms
    auto leftArm = std::make_unique<Limb>("Left Arm", Limb::Type::ARM, 30, -0.5f, 0.0f, 0.2f, 0.7f);
    anatomy.addBodyPart(std::move(leftArm));

    auto rightArm = std::make_unique<Limb>("Right Arm", Limb::Type::ARM, 30, 0.5f, 0.0f, 0.2f, 0.7f);
    anatomy.addBodyPart(std::move(rightArm));

    // 4. Legs
    auto leftLeg = std::make_unique<Limb>("Left Leg", Limb::Type::LEG, 30, -0.2f, 0.6f, 0.2f, 0.8f);
    anatomy.addBodyPart(std::move(leftLeg));

    auto rightLeg = std::make_unique<Limb>("Right Leg", Limb::Type::LEG, 30, 0.2f, 0.6f, 0.2f, 0.8f);
    anatomy.addBodyPart(std::move(rightLeg));

    return entity;
}
