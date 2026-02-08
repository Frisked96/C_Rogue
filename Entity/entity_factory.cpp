#include "entity_factory.hpp"
#include "anatomy_components.hpp"
#include "biological_tags.hpp"
#include "components.hpp"
#include "components/limb.hpp"
#include "components/heart.hpp"
#include "components/lung.hpp"
#include "components/brain.hpp"
#include "components/eye.hpp"
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
    auto torso = Limb::create("Torso", LimbType::NONE, 50, 0.6f, 0.8f, 0.3f);
    torso->self.is_vital = true;

    // Organs in Torso
    torso->addInternalPart(Heart::createBiological());
    torso->addInternalPart(Lung::createHuman("Left Lung"));
    torso->addInternalPart(Lung::createHuman("Right Lung"));

    // 2. Head
    auto head = Limb::create("Head", LimbType::HEAD, 20, 0.3f, 0.3f, 0.25f);
    head->self.is_vital = true;

    // Organs in Head
    head->addInternalPart(Brain::createHuman());
    head->addInternalPart(Eye::createHuman("Left Eye"), -0.05f, 0.05f, 0.1f);
    head->addInternalPart(Eye::createHuman("Right Eye"), 0.05f, 0.05f, 0.1f);

    // Attach Head to Torso
    torso->addChildLimb(head, 0.0f, -0.6f, 0.0f);

    // 3. Arms
    auto lArm = Limb::create("Left Arm", LimbType::ARM, 30, 0.2f, 0.7f, 0.15f);
    torso->addChildLimb(lArm, -0.5f, 0.0f, 0.0f);

    auto rArm = Limb::create("Right Arm", LimbType::ARM, 30, 0.2f, 0.7f, 0.15f);
    torso->addChildLimb(rArm, 0.5f, 0.0f, 0.0f);

    // 4. Legs
    auto lLeg = Limb::create("Left Leg", LimbType::LEG, 30, 0.2f, 0.8f, 0.15f);
    torso->addChildLimb(lLeg, -0.2f, 0.6f, 0.0f);

    auto rLeg = Limb::create("Right Leg", LimbType::LEG, 30, 0.2f, 0.8f, 0.15f);
    torso->addChildLimb(rLeg, 0.2f, 0.6f, 0.0f);

    // Flatten hierarchy into template
    torso->flatten(t);

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
  entity->addComponent<EnvironmentComponent>();
  entity->addComponent<SpatialProfileComponent>(0.1f, 0.4f, 1.75f);

  // Health? Calculated from body parts or standard?
  // Old code used 100. Let's stick with 100 for now, or sum of vital parts?
  // For now 100 to maintain compatibility.
  entity->addComponent<HealthComponent>(100);

  auto &anatomy = entity->addComponent<AnatomyComponent>();
  anatomy.physiology_config = bodyTemplate.physiology;

  // Pass 1: Create parts
  for (const auto &tp : bodyTemplate.parts) {
      BodyPart part(tp.name, tp.max_hp, tp.is_vital, tp.armor, tp.width, tp.height, tp.depth);
      part.type = tp.type;
      part.limb_type = tp.limb_type;
      part.organ_type = tp.organ_type;
      part.relative_x = tp.relative_x;
      part.relative_y = tp.relative_y;
      part.relative_z = tp.relative_z;
      part.base_relative_x = tp.relative_x;
      part.base_relative_y = tp.relative_y;
      part.base_relative_z = tp.relative_z;
      part.strength = tp.strength;
      part.dexterity = tp.dexterity;
      part.efficiency = tp.efficiency;
      part.tags = tp.tags;

      anatomy.addBodyPart(part);
  }

  // Pass 2: Resolve Hierarchy
  for (int i = 0; i < (int)bodyTemplate.parts.size(); ++i) {
      const auto &tp = bodyTemplate.parts[i];
      int parentIdx = -1;

      if (tp.parent_index != -1) {
          parentIdx = tp.parent_index;
      } else if (tp.parent_name != "ROOT" && !tp.parent_name.empty()) {
          parentIdx = anatomy.getBodyPartIndex(tp.parent_name);
      }

      if (parentIdx != -1 && parentIdx < (int)anatomy.body_parts.size()) {
          anatomy.body_parts[i].parent_index = parentIdx;
          anatomy.body_parts[parentIdx].children_indices.push_back(i);
      }
  }

  return entity;
}
