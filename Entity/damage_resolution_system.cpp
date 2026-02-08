#include "damage_resolution_system.hpp"
#include <iostream>

AttackResult DamageResolutionSystem::resolveAttack(Entity* attacker, Entity* defender, const DamageInfo& info) {
    AttackResult result;
    
    auto* anatomy = defender->getComponent<AnatomyComponent>();
    auto* health = defender->getComponent<HealthComponent>();
    auto* defCombat = defender->getComponent<CombatComponent>(); // To get defender's reach/defense
    
    if (!anatomy || !health || !health->is_alive) return result;

    // 0. Calculate Hit Chance (New Logic)
    float baseHitChance = 0.8f;
    float accuracy = 1.0f; // Default if no combat component on attacker
    
    // Attempt to get attacker accuracy from component if available
    if (attacker) {
        if (auto* atkCombat = attacker->getComponent<CombatComponent>()) {
            accuracy = atkCombat->accuracy;
        }
    }

    // Calculate Reach Advantage
    float defReach = 0.5f; // Default (fists/short)
    float defDefense = 0.0f;
    if (defCombat) {
        defReach = defCombat->reach;
        defDefense = static_cast<float>(defCombat->defense); // Using defense as dodge/parry score
    }

    float reachDelta = info.reach - defReach;
    float reachBonus = reachDelta * 0.15f; // +15% hit chance per 1.0 reach advantage

    // Leverage vs Defense (Leverage helps break through guards)
    // If leverage > 1.0, it reduces the effective defense of the target for the hit calculation
    float effectiveDefense = defDefense / info.leverage;

    // Final Hit Calculation
    // Simple formula: (Base + Acc + ReachBonus) - (Defense * 0.05)
    // Assuming Defense is like D&D AC or similar, let's scale it. 
    // If Defense is 5, 5 * 0.05 = 0.25 reduction.
    float hitChance = (baseHitChance * accuracy) + reachBonus - (effectiveDefense * 0.05f);

    std::uniform_real_distribution<float> hitRoll(0.0f, 1.0f);
    if (hitRoll(rng) > hitChance) {
        result.hit = false;
        return result; // Missed
    }

    result.hit = true;

    // 1. Select target body part
    BodyPart* targetPart = selectTargetPart(anatomy, info.type);
    if (!targetPart) return result;
    
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

    // Only consider root parts (or large surface area parts) initially?
    // Or consider all parts? Let's consider all parts for simplicity first, weighted by size.
    // However, internal organs shouldn't be hit directly unless penetration happens.
    // So filter for parts with no parent (root) or parts that are explicitly "external".
    // For now, let's assume all parts are candidates but weight them.
    // Actually, hitting the Heart directly without hitting Chest is wrong.
    // So we should select from Root parts, then drill down.

    for (auto& part : anatomy->body_parts) {
        if (part.parent_index == -1) { // Root parts (Torso, Head, Limbs usually)
            candidates.push_back(&part);
            weights.push_back(part.getTargetWeight());
        }
    }

    if (candidates.empty()) return nullptr;

    std::discrete_distribution<int> dist(weights.begin(), weights.end());
    BodyPart* selected = candidates[dist(rng)];

    // Chance to hit internal parts (e.g., piercing has higher chance)
    float internalChance = 0.1f;
    if (type == DamageType::PIERCING) internalChance = 0.4f;
    
    std::uniform_real_distribution<float> roll(0.0f, 1.0f);
    
    // Drill down into children
    while (!selected->children_indices.empty() && roll(rng) < internalChance) {
        std::vector<BodyPart*> internals;
        std::vector<float> internalWeights;

        for (int childIdx : selected->children_indices) {
            BodyPart& child = anatomy->body_parts[childIdx];
            internals.push_back(&child);
            internalWeights.push_back(child.getTargetWeight());
        }

        if (internals.empty()) break;

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
    // In a flat system, we can look up parent index directly
    if (part->parent_index != -1) {
        // Apply half of excess damage to parent?
        // We'd need access to anatomy component here, but we only have part* and entity*.
        // Entity* has anatomy.
        if (auto* anatomy = entity->getComponent<AnatomyComponent>()) {
             BodyPart& parent = anatomy->body_parts[part->parent_index];
             parent.takeDamage(static_cast<int>(excessDamage * 0.5f));
        }
    }
}
