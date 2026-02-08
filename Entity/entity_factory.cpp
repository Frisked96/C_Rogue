#include "entity_factory.hpp"
#include "components.hpp"
#include "anatomy_components.hpp"
#include <memory>
#include <vector>

EntityFactory::EntityFactory(EntityManager& em) : entityManager(em) {}

Entity* EntityFactory::createPlayer(int x, int y, const std::string& name, char glyph) {
    // Create a generic entity instead of a specific Player class
    Entity* entity = entityManager.createEntity();

    // Add components that define a player
    entity->addComponent<PositionComponent>(x, y);
    entity->addComponent<RenderComponent>(glyph); // Z-index not supported in this component version
    entity->addComponent<NameComponent>(name);
    entity->addComponent<BlockingComponent>(); // No args needed for tag component
    
    // Add health component (standard for player)
    auto& health = entity->addComponent<HealthComponent>(100);

    // Setup Anatomy with flat structure
    auto& anatomy = entity->addComponent<AnatomyComponent>();

    // 1. Torso (Root)
    BodyPart torso("Torso", 50, true);
    torso.width = 0.6f;
    torso.height = 0.8f;
    int torsoIdx = anatomy.addBodyPart(torso);

    // 2. Head (Root)
    BodyPart head("Head", 20, true); // Vital
    head.type = BodyPartType::LIMB;
    head.limb_type = LimbType::HEAD;
    head.width = 0.3f;
    head.height = 0.3f;
    head.relative_y = -0.6f;
    int headIdx = anatomy.addBodyPart(head);

    // 3. Organs inside Torso
    BodyPart heart("Heart", 10, true);
    heart.type = BodyPartType::ORGAN;
    heart.organ_type = OrganType::HEART;
    heart.width = 0.15f;
    heart.height = 0.15f;
    anatomy.addChildPart(torsoIdx, heart);

    BodyPart lungs("Lungs", 15, true);
    lungs.type = BodyPartType::ORGAN;
    lungs.organ_type = OrganType::LUNG;
    lungs.width = 0.2f;
    lungs.height = 0.2f;
    anatomy.addChildPart(torsoIdx, lungs);

    // 4. Brain inside Head
    BodyPart brain("Brain", 5, true);
    brain.type = BodyPartType::ORGAN;
    brain.organ_type = OrganType::BRAIN;
    brain.width = 0.1f;
    brain.height = 0.1f;
    anatomy.addChildPart(headIdx, brain);

    // 5. Limbs
    // Left Arm
    BodyPart lArm("Left Arm", 30);
    lArm.type = BodyPartType::LIMB;
    lArm.limb_type = LimbType::ARM;
    lArm.width = 0.2f;
    lArm.height = 0.7f;
    lArm.relative_x = -0.5f;
    anatomy.addBodyPart(lArm);

    // Right Arm
    BodyPart rArm("Right Arm", 30);
    rArm.type = BodyPartType::LIMB;
    rArm.limb_type = LimbType::ARM;
    rArm.width = 0.2f;
    rArm.height = 0.7f;
    rArm.relative_x = 0.5f;
    anatomy.addBodyPart(rArm);

    // Left Leg
    BodyPart lLeg("Left Leg", 30);
    lLeg.type = BodyPartType::LIMB;
    lLeg.limb_type = LimbType::LEG;
    lLeg.width = 0.2f;
    lLeg.height = 0.8f;
    lLeg.relative_x = -0.2f;
    lLeg.relative_y = 0.6f;
    anatomy.addBodyPart(lLeg);

    // Right Leg
    BodyPart rLeg("Right Leg", 30);
    rLeg.type = BodyPartType::LIMB;
    rLeg.limb_type = LimbType::LEG;
    rLeg.width = 0.2f;
    rLeg.height = 0.8f;
    rLeg.relative_x = 0.2f;
    rLeg.relative_y = 0.6f;
    anatomy.addBodyPart(rLeg);

    return entity;
}
