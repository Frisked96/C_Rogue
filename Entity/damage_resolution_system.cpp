#include "damage_resolution_system.hpp"
#include <iostream>

AttackResult DamageResolutionSystem::resolveAttack(Entity* attacker, Entity* defender, const DamageInfo& info) {
    AttackResult result;
    
    auto* anatomy = defender->getComponent<AnatomyComponent>();
    auto* health = defender->getComponent<HealthComponent>();
    
    if (!anatomy || !health || !health->is_alive) return result;

    // 1. Select target body part
    BodyPart* targetPart = selectTargetPart(anatomy, info.type);
    if (!targetPart) return result;
    
    result.hit = true;
    result.body_part_name = targetPart->name;

    // 2. Armor Calculation
    float armorReduction = calculateArmorReduction(targetPart, info);
    float finalDamage = info.amount - armorReduction;
    if (finalDamage < 0) finalDamage = 0;

    // 3. Critical Check (Targeting vital organs)
    if (targetPart->is_vital) {
        finalDamage *= info.critical_multiplier;
        result.critical = true;
    }

    // 4. Apply Damage to Body Part
    float damageToApply = finalDamage;
    int previousHp = targetPart->current_hitpoints;
    targetPart->takeDamage(static_cast<int>(damageToApply));
    
    result.damage_dealt = static_cast<float>(previousHp - targetPart->current_hitpoints);
    if (targetPart->current_hitpoints <= 0 && previousHp > 0) {
        result.part_destroyed = true;
    }

    // 5. Apply Secondary Effects (Pain, Bleeding, Arterial Integrity)
    applySecondaryEffects(targetPart, info, result.damage_dealt, result);

    // 6. Global Health Impact
    health->takeDamage(static_cast<int>(result.damage_dealt));

    // 7. Handle Overflow
    if (finalDamage > result.damage_dealt) {
        handleDamageOverflow(defender, targetPart, finalDamage - result.damage_dealt);
    }

    return result;
}

BodyPart* DamageResolutionSystem::selectTargetPart(AnatomyComponent* anatomy, DamageType type) {
    if (anatomy->body_parts.empty()) return nullptr;

    // Weighted selection based on size
    std::vector<BodyPart*> candidates;
    std::vector<float> weights;

    for (const auto& part : anatomy->body_parts) {
        candidates.push_back(part.get());
        weights.push_back(part->getTargetWeight());
    }

    std::discrete_distribution<int> dist(weights.begin(), weights.end());
    BodyPart* selected = candidates[dist(rng)];

    // Chance to hit internal parts (e.g., piercing has higher chance)
    float internalChance = 0.1f;
    if (type == DamageType::PIERCING) internalChance = 0.4f;
    
    std::uniform_real_distribution<float> roll(0.0f, 1.0f);
    
    while (!selected->internal_parts.empty() && roll(rng) < internalChance) {
        std::vector<BodyPart*> internals;
        std::vector<float> internalWeights;
        for (const auto& internal : selected->internal_parts) {
            internals.push_back(internal.get());
            internalWeights.push_back(internal->getTargetWeight());
        }
        std::discrete_distribution<int> iDist(internalWeights.begin(), internalWeights.end());
        selected = internals[iDist(rng)];
        
        // Decay chance for deeper penetration
        internalChance *= 0.5f; 
    }

    return selected;
}

float DamageResolutionSystem::calculateArmorReduction(BodyPart* part, const DamageInfo& info) {
    float effectiveArmor = static_cast<float>(part->armor_value) * (1.0f - info.armor_penetration);
    
    // Blunt damage is less affected by armor but still reduced
    if (info.type == DamageType::BLUNT) {
        effectiveArmor *= 0.7f; 
    }
    
    return effectiveArmor;
}

void DamageResolutionSystem::applySecondaryEffects(BodyPart* part, const DamageInfo& info, float damageDealt, AttackResult& result) {
    // 1. Pain calculation
    float painFactor = 1.0f;
    if (info.type == DamageType::BLUNT) painFactor = 1.5f;
    part->pain_level += (damageDealt / part->max_hitpoints) * 50.0f * painFactor;
    if (part->pain_level > 100.0f) part->pain_level = 100.0f;
    result.pain_inflicted = part->pain_level;

    // 2. Bleeding
    float bleedingChance = 0.0f;
    if (info.type == DamageType::SHARP) bleedingChance = 0.8f;
    else if (info.type == DamageType::PIERCING) bleedingChance = 0.4f;
    else if (info.type == DamageType::BLUNT) bleedingChance = 0.1f;

    std::uniform_real_distribution<float> roll(0.0f, 1.0f);
    if (roll(rng) < bleedingChance) {
        int intensity = static_cast<int>(damageDealt / 5.0f);
        if (intensity < 1) intensity = 1;
        part->bleeding_intensity += intensity;
    }

    // 3. Arterial Integrity
    if (info.type == DamageType::SHARP || info.type == DamageType::PIERCING) {
        float arterialHitChance = (damageDealt / part->max_hitpoints) * 0.5f;
        if (roll(rng) < arterialHitChance) {
            part->arterial_integrity -= 0.3f;
            if (part->arterial_integrity < 0.0f) part->arterial_integrity = 0.0f;
            result.arterial_hit = true;
        }
    }
}

void DamageResolutionSystem::handleDamageOverflow(Entity* entity, BodyPart* part, float excessDamage) {
    // In a more complex system, we would find the parent part.
    // For now, apply directly to HealthComponent which we already did in resolveAttack.
}
