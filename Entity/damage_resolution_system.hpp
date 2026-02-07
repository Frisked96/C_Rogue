#pragma once
#include "entity_manager.hpp"
#include "anatomy_components.hpp"
#include "components.hpp"
#include "combat_types.hpp"
#include <random>

/**
 * @brief System responsible for resolving attacks with anatomical precision.
 */
class DamageResolutionSystem {
public:
    DamageResolutionSystem() : rng(std::random_device{}()) {}

    /**
     * @brief Resolves an attack from an attacker to a defender.
     * @param attacker The entity performing the attack.
     * @param defender The entity receiving the attack.
     * @param info Detailed damage information from the weapon/source.
     * @return Result of the attack resolution.
     */
    AttackResult resolveAttack(Entity* attacker, Entity* defender, const DamageInfo& info);

private:
    std::mt19937 rng;

    /**
     * @brief Recursively selects a target body part based on size and accessibility.
     */
    BodyPart* selectTargetPart(AnatomyComponent* anatomy, DamageType type);

    /**
     * @brief Calculates armor reduction for a specific part and damage type.
     */
    float calculateArmorReduction(BodyPart* part, const DamageInfo& info);

    /**
     * @brief Applies specific anatomical effects based on damage type (bleeding, pain, etc).
     */
    void applySecondaryEffects(BodyPart* part, const DamageInfo& info, float damageDealt, AttackResult& result);

    /**
     * @brief Distributes excess damage to parent parts if the target part is destroyed.
     */
    void handleDamageOverflow(Entity* entity, BodyPart* part, float excessDamage);
};
