#pragma once
#include "component.hpp"
#include "combat_types.hpp"
#include <string>

// basic components for most entities
class PositionComponent : public BaseComponent<PositionComponent> {
public:
  int x;
  int y;

  PositionComponent(int x = 0, int y = 0) : x(x), y(y) {}
};

class RenderComponent : public BaseComponent<RenderComponent> {
public:
  char glyph;

  RenderComponent(char glyph = '?') : glyph(glyph) {}
};

class NameComponent : public BaseComponent<NameComponent> {
public:
  std::string name;

  NameComponent(const std::string &name = "") : name(name) {}
};

class BlockingComponent : public BaseComponent<BlockingComponent> {
  // No data as this is just a flag
  // entities that have this block movement
};

class HealthComponent : public BaseComponent<HealthComponent> {
public:
  int current_health;
  int max_health;
  bool is_alive;

  HealthComponent(int max_hp = 100)
      : current_health(max_hp), max_health(max_hp), is_alive(true) {}

  void takeDamage(int damage) {
    current_health -= damage;
    if (current_health <= 0) {
      current_health = 0;
      is_alive = false;
    }
  }

  void heal(int amount) {
    current_health += amount;
    if (current_health > max_health) {
      current_health = max_health;
    }
    if (current_health > 0) {
      is_alive = true;
    }
  }

  float getHEalthPercentage() const {
    return static_cast<float>(current_health) / max_health;
  }
};

class CombatComponent : public BaseComponent<CombatComponent> {
public:
  int attack_damage;
  int defense;
  float attack_range;
  DamageType preferred_damage_type;
  float accuracy;

  CombatComponent(int atk = 10, int def = 5, float range = 1.0f, DamageType type = DamageType::BLUNT)
      : attack_damage(atk), defense(def), attack_range(range), preferred_damage_type(type), accuracy(1.0f) {}

  DamageInfo getDamageInfo() const {
      return DamageInfo(static_cast<float>(attack_damage), preferred_damage_type);
  }
};

class FactionComponent : public BaseComponent<FactionComponent> {
public:
  enum class Faction { PLAYER, MONSTER, NEUTRAL, FRIENDLY };

  Faction faction;
  bool hostile_to_player;

  FactionComponent(Faction f = Faction::NEUTRAL, bool hostile = false)
      : faction(f), hostile_to_player(hostile) {}
};