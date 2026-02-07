#pragma once
#include <string>

/**
 * @brief Types of damage that can be dealt.
 * Each type has different interactions with armor and body parts.
 */
enum class DamageType {
    BLUNT,    // Good against hard armor, high pain, internal organ damage
    SHARP,    // Causes high bleeding, potential to sever limbs
    PIERCING, // High armor penetration, high chance to hit internal organs
    ENERGY,   // Bypasses physical armor, affects physiology directly
    ACID      // Corrodes armor and causes lingering damage
};

/**
 * @brief Category of the attack.
 */
enum class AttackType {
    MELEE,
    RANGED,
    MAGIC,
    ENVIRONMENTAL,
    SYSTEMIC // e.g., poison, hunger
};

/**
 * @brief Detailed information about an incoming attack or damage source.
 */
struct DamageInfo {
    float amount;
    DamageType type;
    float armor_penetration; // 0.0 (none) to 1.0 (full bypass)
    float critical_multiplier;
    AttackType attack_type;

    DamageInfo(float amt = 0.0f, 
               DamageType t = DamageType::BLUNT, 
               float pen = 0.0f, 
               float crit = 2.0f,
               AttackType at = AttackType::MELEE)
        : amount(amt), type(t), armor_penetration(pen), 
          critical_multiplier(crit), attack_type(at) {}
};

/**
 * @brief Result of a resolved attack.
 */
struct AttackResult {
    bool hit;
    bool critical;
    float damage_dealt;
    std::string body_part_name;
    bool part_destroyed;
    bool arterial_hit;
    float pain_inflicted;

    AttackResult() 
        : hit(false), critical(false), damage_dealt(0.0f), 
          body_part_name(""), part_destroyed(false), 
          arterial_hit(false), pain_inflicted(0.0f) {}
};
