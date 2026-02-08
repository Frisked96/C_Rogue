#include "entity_factory.hpp"
#include "anatomy_components.hpp"
#include "biological_tags.hpp"
#include "components.hpp"
#include <memory>
#include <vector>

EntityFactory::EntityFactory(EntityManager &em) : entityManager(em) {}

Entity *EntityFactory::createPlayer(int x, int y, const std::string &name,
                                    char glyph) {
    return createFromTemplate(x, y, createHumanTemplate(), name, glyph);
}

BodyTemplate EntityFactory::createHumanTemplate() {
    BodyTemplate t;
    t.template_name = "Human";
    // Default PhysiologyConfig is already human-like

    // 1. Torso (Root)
    BodyTemplatePart torso("Torso", "ROOT", 50, BodyPartType::GENERIC);
    torso.width = 0.6f;
    torso.height = 0.8f;
    torso.is_vital = true;
    t.addPart(torso);

    // 2. Head (Root, visually attached to top of torso area but logically separate root in old code?
    // In old code: head.relative_y = -0.6f; parent_index = -1 (implicit).
    // So yes, "ROOT".
    BodyTemplatePart head("Head", "ROOT", 20, BodyPartType::LIMB);
    head.limb_type = LimbType::HEAD;
    head.width = 0.3f;
    head.height = 0.3f;
    head.relative_y = -0.6f;
    head.is_vital = true;
    t.addPart(head);

    // 3. Organs inside Torso
    BodyTemplatePart heart("Heart", "Torso", 10, BodyPartType::ORGAN);
    heart.organ_type = OrganType::HEART;
    heart.width = 0.15f;
    heart.height = 0.15f;
    heart.is_vital = true;
    heart.tags.push_back(BioTags::CIRCULATION);
    t.addPart(heart);

    BodyTemplatePart lungs("Lungs", "Torso", 15, BodyPartType::ORGAN);
    lungs.organ_type = OrganType::LUNG;
    lungs.width = 0.2f;
    lungs.height = 0.2f;
    lungs.is_vital = true;
    lungs.tags.push_back(BioTags::RESPIRATION);
    t.addPart(lungs);

    // 4. Brain inside Head
    BodyTemplatePart brain("Brain", "Head", 5, BodyPartType::ORGAN);
    brain.organ_type = OrganType::BRAIN;
    brain.width = 0.1f;
    brain.height = 0.1f;
    brain.is_vital = true;
    brain.tags.push_back(BioTags::NEURAL);
    t.addPart(brain);

    // 5. Limbs
    // Left Arm
    BodyTemplatePart lArm("Left Arm", "ROOT", 30, BodyPartType::LIMB);
    lArm.limb_type = LimbType::ARM;
    lArm.width = 0.2f;
    lArm.height = 0.7f;
    lArm.relative_x = -0.5f;
    t.addPart(lArm);

    // Right Arm
    BodyTemplatePart rArm("Right Arm", "ROOT", 30, BodyPartType::LIMB);
    rArm.limb_type = LimbType::ARM;
    rArm.width = 0.2f;
    rArm.height = 0.7f;
    rArm.relative_x = 0.5f;
    t.addPart(rArm);

    // Left Leg
    BodyTemplatePart lLeg("Left Leg", "ROOT", 30, BodyPartType::LIMB);
    lLeg.limb_type = LimbType::LEG;
    lLeg.width = 0.2f;
    lLeg.height = 0.8f;
    lLeg.relative_x = -0.2f;
    lLeg.relative_y = 0.6f;
    t.addPart(lLeg);

    // Right Leg
    BodyTemplatePart rLeg("Right Leg", "ROOT", 30, BodyPartType::LIMB);
    rLeg.limb_type = LimbType::LEG;
    rLeg.width = 0.2f;
    rLeg.height = 0.8f;
    rLeg.relative_x = 0.2f;
    rLeg.relative_y = 0.6f;
    t.addPart(rLeg);

    return t;
}

Entity *EntityFactory::createFromTemplate(int x, int y, const BodyTemplate &bodyTemplate,
                                          const std::string &name, char glyph) {
  Entity *entity = entityManager.createEntity();

  // Add standard components
  entity->addComponent<PositionComponent>(x, y);
  entity->addComponent<RenderComponent>(glyph);
  entity->addComponent<NameComponent>(name);
  entity->addComponent<BlockingComponent>();

  // Health? Calculated from body parts or standard?
  // Old code used 100. Let's stick with 100 for now, or sum of vital parts?
  // For now 100 to maintain compatibility.
  entity->addComponent<HealthComponent>(100);

  auto &anatomy = entity->addComponent<AnatomyComponent>();
  anatomy.physiology_config = bodyTemplate.physiology;

  // Pass 1: Create parts
  for (const auto &tp : bodyTemplate.parts) {
      BodyPart part(tp.name, tp.max_hp, tp.is_vital, tp.armor, tp.width, tp.height);
      part.type = tp.type;
      part.limb_type = tp.limb_type;
      part.organ_type = tp.organ_type;
      part.relative_x = tp.relative_x;
      part.relative_y = tp.relative_y;
      part.strength = tp.strength;
      part.dexterity = tp.dexterity;
      part.efficiency = tp.efficiency;
      part.tags = tp.tags;

      anatomy.addBodyPart(part);
  }

  // Pass 2: Resolve Hierarchy
  for (int i = 0; i < (int)bodyTemplate.parts.size(); ++i) {
      const auto &tp = bodyTemplate.parts[i];
      if (tp.parent_name != "ROOT" && !tp.parent_name.empty()) {
          int parentIdx = anatomy.getBodyPartIndex(tp.parent_name);
          if (parentIdx != -1) {
              anatomy.body_parts[i].parent_index = parentIdx;
              anatomy.body_parts[parentIdx].children_indices.push_back(i);
          }
      }
  }

  return entity;
}
