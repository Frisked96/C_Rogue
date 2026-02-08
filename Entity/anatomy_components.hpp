#pragma once
#include "component.hpp"
#include "anatomy_defs.hpp"
#include <memory>
#include <string>
#include <vector>

// Flattened BodyPart class
class BodyPart {
public:
  // Identification
  std::string name;
  BodyPartType type;
  LimbType limb_type;
  OrganType organ_type;

  // Status
  int max_hitpoints;
  int current_hitpoints;
  bool is_vital;
  bool is_immune_to_poison;
  bool is_functional;
  int bleeding_intensity;   // Damage per turn due to bleeding
  float pain_level;         // 0.0 to 100.0
  float arterial_integrity; // 1.0 = intact, 0.0 = severed
  int armor_value;          // Damage reduction

  // Spatial information relative to parent
  float relative_x;
  float relative_y;
  float width;
  float height;

  // Hierarchy (Indices into AnatomyComponent::body_parts)
  int parent_index; // -1 if root
  std::vector<int> children_indices;

  // Specific Properties (Merged from Limb/Organ)
  float strength;   // Limb
  float dexterity;  // Limb (corrected spelling)
  float efficiency; // Organ (0.0 to 1.0)

  // Attachments
  std::vector<std::string> attachment_points;

  // Semantic Tags (BioTags)
  std::vector<std::string> tags;

  BodyPart()
      : name(""), type(BodyPartType::GENERIC), limb_type(LimbType::NONE),
        organ_type(OrganType::NONE), max_hitpoints(0), current_hitpoints(0),
        is_vital(false), is_immune_to_poison(false), is_functional(true),
        bleeding_intensity(0), pain_level(0.0f), arterial_integrity(1.0f),
        armor_value(0), relative_x(0), relative_y(0), width(0), height(0),
        parent_index(-1), strength(0.0f), dexterity(0.0f), efficiency(1.0f) {}

  BodyPart(const std::string &name, int hp, bool vital = false, int armor = 0,
           float w = 0.5f, float h = 0.5f)
      : name(name), type(BodyPartType::GENERIC), limb_type(LimbType::NONE),
        organ_type(OrganType::NONE), max_hitpoints(hp), current_hitpoints(hp),
        is_vital(vital), is_immune_to_poison(false), is_functional(true),
        bleeding_intensity(0), pain_level(0.0f), arterial_integrity(1.0f),
        armor_value(armor), relative_x(0), relative_y(0), width(w), height(h),
        parent_index(-1), strength(0.0f), dexterity(0.0f), efficiency(1.0f) {}

  float getTargetWeight() const { return width * height; }

  bool canFunction() const { return is_functional && current_hitpoints > 0; }

  void takeDamage(int damage) {
    current_hitpoints -= damage;
    if (current_hitpoints <= 0) {
      is_functional = false;
      current_hitpoints = 0;
    }
    // Update efficiency for organs if damaged
    if (type == BodyPartType::ORGAN) {
      efficiency = static_cast<float>(current_hitpoints) / max_hitpoints;
    }
  }

  void heal(int amount) {
    current_hitpoints += amount;
    if (current_hitpoints > max_hitpoints) {
      current_hitpoints = max_hitpoints;
    }
    if (current_hitpoints > 0) {
      is_functional = true;
    }
    if (type == BodyPartType::ORGAN) {
      efficiency = static_cast<float>(current_hitpoints) / max_hitpoints;
    }
  }

  void addTag(const std::string &tag) { tags.push_back(tag); }

  bool hasTag(const std::string &tag) const {
    for (const auto &t : tags) {
      if (t == tag)
        return true;
    }
    return false;
  }
};

class AnatomyComponent : public BaseComponent<AnatomyComponent> {
public:
  std::vector<BodyPart> body_parts;

  // Physiology Configuration
  PhysiologyConfig physiology_config;

  // Physiology State
  float blood_volume;
  float max_blood_volume;
  float oxygen_saturation;
  float stored_energy;
  float hydration;
  float accumulated_pain;
  float stress_level;
  float adrenaline_level;

  AnatomyComponent()
      : blood_volume(5.0f), max_blood_volume(5.0f), oxygen_saturation(100.0f),
        stored_energy(2000.0f), hydration(100.0f), accumulated_pain(0.0f),
        stress_level(0.0f), adrenaline_level(0.0f) {}

  std::unique_ptr<Component> clone() const override;

  // Add a body part (returns index)
  int addBodyPart(const BodyPart &part);

  // Add a child part to a parent (returns child index)
  int addChildPart(int parentIndex, const BodyPart &part);

  // Find a body part by name
  BodyPart *getBodyPart(const std::string &name);
  int getBodyPartIndex(const std::string &name);

  // Check if anatomy is functional (at least one vital organ working)
  bool isFunctional() const;

  // Take damage to a body part
  void takeDamageToBodyPart(const std::string &part_name, int damage);

  // Replace a body part at runtime
  void replaceBodyPart(int index, const BodyPart &newPart);
};
