#pragma once
#include "component.hpp"
#include <memory>
#include <string>
#include <vector>

class BodyPart;

class AnatomyComponent : public BaseComponent<AnatomyComponent> {
public:
  std::vector<std::unique_ptr<BodyPart>> body_parts;

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

  // Attachments this part can have (eg. armor, weapons, hand can hold items)
  std::vector<std::string> attachment_points;

  BodyPart(const std::string &name, int hp, bool vital = false,
           bool immune_to_poison = false)
      : name(name), max_hitpoints(hp), current_hitpoints(hp), is_vital(vital),
        is_immune_to_poison(immune_to_poison), is_functional(true) {}

  virtual ~BodyPart() = default;

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

  Limb(const std::string &name, Type type, int hp, float str = 1.0f,
       float dex = 1.0f, bool vital = false)
      : BodyPart(name, hp), limb_type(type), strength(str), dexerity(dex) {
    // arm can hold things
    if (type == Type::ARM) {
      attachment_points.push_back("hand");
    }
  }
};

class Organ : public BodyPart {
public:
  enum class Type { Heart, Lung, Brain, STOMACH, LIVER, KIDNEY };

  Type organ_type;
  float efficiency; // 0.0 failed , 1.0 perfect

  Organ(const std::string &name, Type type, int hp, bool vital = true,
        float eff = 1.0f)
      : BodyPart(name, hp), organ_type(type), efficiency(eff) {}

  void takeDamage(int damage) override {
    BodyPart::takeDamage(damage);
    // damage reduces efficiency of organ
    efficiency = static_cast<float>(current_hitpoints) / max_hitpoints;
  }
};