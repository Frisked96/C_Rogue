#pragma once
#include "component.hpp"
#include <memory>
#include <string>
#include <vector>

class BodyPart;

class AnatomyComponent : public BaseComponent<AnatomyComponent> {
public:
  std::vector<std::unique_ptr<BodyPart>> body_parts;

  // Physiology State
  float blood_volume;        // Liters (e.g. 5.0 for human)
  float max_blood_volume;
  float oxygen_saturation;   // 0.0 to 100.0
  float stored_energy;       // Calories/Energy units
  float hydration;           // 0.0 to 100.0
  float accumulated_pain;    // Sum of pain from all parts

  AnatomyComponent() 
      : blood_volume(5.0f), max_blood_volume(5.0f), 
        oxygen_saturation(100.0f), stored_energy(2000.0f), 
        hydration(100.0f), accumulated_pain(0.0f) {}

  // Custom clone because of unique_ptr
  std::unique_ptr<Component> clone() const override;

  // Add a body part
  void addBodyPart(std::unique_ptr<BodyPart> part);

  // find a body part by name
  BodyPart *getBodyPart(const std::string &name);

  // check if anatomy is functional atleast one vial organ working
  bool isFunctional() const;

  // Take damage to a body part
  void takeDamageToBodyPart(const std::string &part_name, int damage);
};

// base body part class
class BodyPart : public BaseComponent<BodyPart> {
public:
  std::string name;
  int max_hitpoints;
  int current_hitpoints;
  bool is_vital;
  bool is_immune_to_poison;
  bool is_functional;
  int bleeding_intensity; // Damage per turn due to bleeding

  // Spatial information relative to parent (or entity center if root)
  float relative_x;
  float relative_y;
  float width;  // replacing hit_radius
  float height; // replacing hit_radius

  // Container hierarchy
  std::vector<std::unique_ptr<BodyPart>> internal_parts;

  // Attachments this part can have (eg. armor, weapons, hand can hold items)
  std::vector<std::string> attachment_points;

  BodyPart() : name(""), max_hitpoints(0), current_hitpoints(0), is_vital(false), is_immune_to_poison(false), is_functional(true), bleeding_intensity(0), pain_level(0.0f), arterial_integrity(1.0f), relative_x(0), relative_y(0), width(0), height(0) {}

  BodyPart(const std::string &name, int hp, bool vital = false,
           bool immune_to_poison = false, float rx = 0.0f, float ry = 0.0f,
           float w = 0.5f, float h = 0.5f)
      : name(name), max_hitpoints(hp), current_hitpoints(hp), is_vital(vital),
        is_immune_to_poison(immune_to_poison), is_functional(true), bleeding_intensity(0),
        pain_level(0.0f), arterial_integrity(1.0f),
        relative_x(rx), relative_y(ry), width(w), height(h) {}

  virtual ~BodyPart() = default;

  virtual std::unique_ptr<BodyPart> clonePart() const {
    auto copy = std::make_unique<BodyPart>(name, max_hitpoints, is_vital,
                                          is_immune_to_poison, relative_x,
                                          relative_y, width, height);
    copy->current_hitpoints = current_hitpoints;
    copy->is_functional = is_functional;
    copy->bleeding_intensity = bleeding_intensity;
    copy->pain_level = pain_level;
    copy->arterial_integrity = arterial_integrity;
    copy->attachment_points = attachment_points;
    
    // Deep copy internal parts
    for (const auto& part : internal_parts) {
        copy->internal_parts.push_back(part->clonePart());
    }
    
    return copy;
  }

  // Add an internal organ/part
  void addInternalPart(std::unique_ptr<BodyPart> part) {
      internal_parts.push_back(std::move(part));
  }

  // can this part perform its functiom
  virtual bool canFunction() const {
    return is_functional && current_hitpoints > 0;
  }

  // take damage to this part
  virtual void takeDamage(int damage) {
    current_hitpoints -= damage;
    if (current_hitpoints <= 0) {
      is_functional = false;
      current_hitpoints = 0;
    }
  }

  // repair/heal this part
  virtual void heal(int amount) {
    current_hitpoints += amount;
    if (current_hitpoints > max_hitpoints) {
      current_hitpoints = max_hitpoints;
    }
    if (current_hitpoints > 0) {
      is_functional = true;
    }
  }
};

// specialized body parts
class Limb : public BodyPart {
public:
  enum class Type { ARM, LEG, HEAD, TAIL };

  Type limb_type;
  float strength;
  float dexerity;

  Limb(const std::string &name, Type type, int hp, float rx = 0.0f,
       float ry = 0.0f, float w = 0.2f, float h = 0.5f, float str = 1.0f, float dex = 1.0f,
       bool vital = false)
      : BodyPart(name, hp, vital, false, rx, ry, w, h), limb_type(type),
        strength(str), dexerity(dex) {
    // arm can hold things
    if (type == Type::ARM) {
      attachment_points.push_back("hand");
    }
  }

  virtual std::unique_ptr<BodyPart> clonePart() const override {
    auto copy = std::make_unique<Limb>(name, limb_type, max_hitpoints,
                                      relative_x, relative_y, width, height,
                                      strength, dexerity, is_vital);
    copy->current_hitpoints = current_hitpoints;
    copy->is_functional = is_functional;
    copy->bleeding_intensity = bleeding_intensity;
    copy->pain_level = pain_level;
    copy->arterial_integrity = arterial_integrity;
    copy->attachment_points = attachment_points;
    
    for (const auto& part : internal_parts) {
        copy->internal_parts.push_back(part->clonePart());
    }
    
    return copy;
  }
};

class Organ : public BodyPart {
public:
  enum class Type { Heart, Lung, Brain, STOMACH, LIVER, KIDNEY };

  Type organ_type;
  float efficiency; // 0.0 failed , 1.0 perfect

  Organ(const std::string &name, Type type, int hp, float rx = 0.0f,
        float ry = 0.0f, float w = 0.1f, float h = 0.1f, bool vital = true,
        float eff = 1.0f)
      : BodyPart(name, hp, vital, false, rx, ry, w, h), organ_type(type),
        efficiency(eff) {}

  virtual std::unique_ptr<BodyPart> clonePart() const override {
    auto copy = std::make_unique<Organ>(name, organ_type, max_hitpoints,
                                       relative_x, relative_y, width, height,
                                       is_vital, efficiency);
    copy->current_hitpoints = current_hitpoints;
    copy->is_functional = is_functional;
    copy->bleeding_intensity = bleeding_intensity;
    copy->pain_level = pain_level;
    copy->arterial_integrity = arterial_integrity;
    copy->attachment_points = attachment_points;
    
    for (const auto& part : internal_parts) {
        copy->internal_parts.push_back(part->clonePart());
    }

    return copy;
  }

  void takeDamage(int damage) override {
    BodyPart::takeDamage(damage);
    // damage reduces efficiency of organ
    efficiency = static_cast<float>(current_hitpoints) / max_hitpoints;
  }
};