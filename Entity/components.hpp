#pragma once
#include "combat_types.hpp"
#include "component.hpp"
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
  float reach; // Renamed from attack_range for consistency
  DamageType preferred_damage_type;
  float accuracy;
  float leverage; // New field

  CombatComponent(int atk = 10, int def = 5, float rch = 1.0f,
                  DamageType type = DamageType::BLUNT, float lev = 1.0f)
      : attack_damage(atk), defense(def), reach(rch),
        preferred_damage_type(type), accuracy(1.0f), leverage(lev) {}

  DamageInfo getDamageInfo() const {
    return DamageInfo(static_cast<float>(attack_damage), preferred_damage_type,
                      0.0f, 2.0f, AttackType::MELEE, reach, leverage);
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

class EnvironmentComponent : public BaseComponent<EnvironmentComponent> {
public:
  enum class Medium { AIR, WATER, VACUUM };

  float temperature;       // Celsius
  float oxygen_percentage; // 0.0 to 100.0 (Earth norm ~21.0)
  float toxicity_level;    // 0.0 (clean) to 1.0 (deadly)
  Medium medium;

  EnvironmentComponent(float temp = 20.0f, float o2 = 21.0f,
                       float toxin = 0.0f, Medium med = Medium::AIR)
      : temperature(temp), oxygen_percentage(o2), toxicity_level(toxin),
        medium(med) {}
};

class SpatialProfileComponent : public BaseComponent<SpatialProfileComponent> {
public:
  enum class Stance { STANDING, CROUCHING, PRONE, CRAWLING, FLYING };

  float volume_occupancy;     // 0.0 to 1.0 (Percentage of tile volume filled)
  float cross_section_coverage; // 0.0 to 1.0 (Percentage of tile face blocked, for hit calc)
  float height_meters;        // Physical height of the entity
  Stance current_stance;

  SpatialProfileComponent(float vol = 0.5f, float coverage = 0.4f,
                          float h = 1.7f, Stance stance = Stance::STANDING)
      : volume_occupancy(vol), cross_section_coverage(coverage),
        height_meters(h), current_stance(stance) {}
};