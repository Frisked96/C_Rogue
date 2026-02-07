#include "physiology_system.hpp"
#include "components.hpp"
#include <cmath>
#include <algorithm>
#include <iostream> 
#include "entity_manager.hpp"

void PhysiologySystem::processEntity(Entity* entity, AnatomyComponent* anatomy, HealthComponent* health) {
    processCirculation(anatomy, health);
    processRespiration(anatomy);
    processMetabolism(anatomy, health);
    processPain(anatomy); // Updates base pain
    processStress(anatomy); // Updates stress/adrenaline and modifies effective pain
    processHealing(anatomy);
}

// -----------------------------------------------------------------------------
// Circulation & Bleeding
// -----------------------------------------------------------------------------

// Helper to sum bleeding and handle clotting
float updateBleedingRecursive(std::vector<std::unique_ptr<BodyPart>>& parts) {
    float total = 0.0f;
    for (auto& part : parts) {
        total += part->bleeding_intensity;

        // Clotting logic: 10% chance to reduce bleeding
        if (part->bleeding_intensity > 0) {
            if ((rand() % 100) < 10) {
                part->bleeding_intensity = std::max(0, part->bleeding_intensity - 1);
            }
        }
        
        // Check Arterial Integrity - if severed, massive bleeding
        if (part->arterial_integrity < 0.2f) {
             // Force bleeding if artery is severed
             part->bleeding_intensity = std::max(part->bleeding_intensity, 5); 
        }

        if (!part->internal_parts.empty()) {
            total += updateBleedingRecursive(part->internal_parts);
        }
    }
    return total;
}

void PhysiologySystem::processCirculation(AnatomyComponent* anatomy, HealthComponent* health) {
    float totalBleeding = updateBleedingRecursive(anatomy->body_parts);

    // Blood loss
    // Arbitrary scale: 1 intensity = 0.01 liters per turn?
    float bloodLoss = totalBleeding * 0.05f; 
    anatomy->blood_volume -= bloodLoss;
    if (anatomy->blood_volume < 0.0f) anatomy->blood_volume = 0.0f;

    // Direct health damage from massive blood loss (Hypovolemic shock)
    float bloodRatio = anatomy->blood_volume / anatomy->max_blood_volume;
    if (bloodRatio < 0.5f) {
        // Taking damage due to blood loss
        health->takeDamage(1); 
    }

    // Heart efficiency affects circulation (and thus can indirectly affect oxygen)
    float heartEff = calculateOrganEfficiency(anatomy, Organ::Type::Heart);
    if (heartEff < 0.1f && anatomy->blood_volume > 0) {
         // Cardiac arrest - effectively no circulation
         // We represent this by not recovering oxygen and maybe dropping it fast
         // For now, let's say heart failure damages health directly
         health->takeDamage(2);
    }
}

// -----------------------------------------------------------------------------
// Respiration & Oxygen
// -----------------------------------------------------------------------------
void PhysiologySystem::processRespiration(AnatomyComponent* anatomy) {
    float lungEff = calculateOrganEfficiency(anatomy, Organ::Type::Lung);
    float bloodRatio = anatomy->blood_volume / anatomy->max_blood_volume;
    
    // Base oxygen consumption
    float oxygenChange = -2.0f; // Consume 2% per turn

    // Recovery if breathing and have blood
    if (lungEff > 0.5f && bloodRatio > 0.4f) {
        oxygenChange += 5.0f * lungEff * bloodRatio;
    }

    anatomy->oxygen_saturation += oxygenChange;
    
    // Clamp
    if (anatomy->oxygen_saturation > 100.0f) anatomy->oxygen_saturation = 100.0f;
    if (anatomy->oxygen_saturation < 0.0f) anatomy->oxygen_saturation = 0.0f;

    // Hypoxia
    if (anatomy->oxygen_saturation < 30.0f) {
        // Hypoxia damage to vital organs would be cool, but for now simple global damage via health component
        // Or we could damage the Brain specifically?
        // Let's damage the Brain if found
        auto brain = anatomy->getBodyPart("Brain");
        if (brain) {
            brain->takeDamage(1);
        }
    }
}

// -----------------------------------------------------------------------------
// Metabolism
// -----------------------------------------------------------------------------
void PhysiologySystem::processMetabolism(AnatomyComponent* anatomy, HealthComponent* health) {
    // Burn energy
    float metabolicRate = 0.5f; // Calories per turn
    anatomy->stored_energy -= metabolicRate;

    if (anatomy->stored_energy < 0.0f) {
        anatomy->stored_energy = 0.0f;
        // Starvation damage
        health->takeDamage(1);
    }
}

// -----------------------------------------------------------------------------
// Pain
// -----------------------------------------------------------------------------
void calculatePainRecursive(std::vector<std::unique_ptr<BodyPart>>& parts, float& totalPain) {
    for (const auto& part : parts) {
        // Simple model: Pain is proportional to % damage
        if (part->max_hitpoints > 0) {
            float damageRatio = 1.0f - (static_cast<float>(part->current_hitpoints) / part->max_hitpoints);
            // Update local pain level (could be used for limb specific penalties later)
            // part->pain_level = damageRatio * 10.0f; // Scale 0-10 per part
            
            // Add to total
            totalPain += (damageRatio * 10.0f);
        }
        
        calculatePainRecursive(part->internal_parts, totalPain);
    }
}

void PhysiologySystem::processPain(AnatomyComponent* anatomy) {
    float totalPain = 0.0f;
    calculatePainRecursive(anatomy->body_parts, totalPain);
    anatomy->accumulated_pain = totalPain;

    // Shock simulation
    // If pain is very high, maybe skip turn or reduce stats?
    // For now, we just track it. 
}

// -----------------------------------------------------------------------------
// Stress & Adrenaline
// -----------------------------------------------------------------------------
void PhysiologySystem::processStress(AnatomyComponent* anatomy) {
    // 1. Adrenaline Response
    // If pain is high, adrenaline spikes
    if (anatomy->accumulated_pain > 20.0f) {
        float response = (anatomy->accumulated_pain - 20.0f) * 0.1f;
        anatomy->adrenaline_level += response;
    }

    // Decay Adrenaline
    if (anatomy->adrenaline_level > 0.0f) {
        // Decay rate
        float decay = 2.0f;
        // Conversion to stress (Crash)
        anatomy->stress_level += decay * 0.1f;
        
        anatomy->adrenaline_level -= decay;
        if (anatomy->adrenaline_level < 0.0f) anatomy->adrenaline_level = 0.0f;
    }

    // 2. Stress Mechanics
    // Pain causes stress
    if (anatomy->accumulated_pain > 5.0f) {
        anatomy->stress_level += anatomy->accumulated_pain * 0.01f;
    }

    // Decay Stress (slowly, requires safety/rest normally)
    if (anatomy->stress_level > 0.0f) {
        anatomy->stress_level -= 0.1f;
        if (anatomy->stress_level < 0.0f) anatomy->stress_level = 0.0f;
    }

    // Cap
    if (anatomy->stress_level > 100.0f) anatomy->stress_level = 100.0f;
    if (anatomy->adrenaline_level > 100.0f) anatomy->adrenaline_level = 100.0f;

    // 3. Adrenaline Masking Pain
    // High adrenaline masks pain
    if (anatomy->adrenaline_level > 10.0f) {
        float maskAmount = anatomy->adrenaline_level * 0.5f;
        anatomy->accumulated_pain -= maskAmount;
        if (anatomy->accumulated_pain < 0.0f) anatomy->accumulated_pain = 0.0f;
    }
}

// -----------------------------------------------------------------------------
// Healing
// -----------------------------------------------------------------------------
bool tryHealRecursive(std::vector<std::unique_ptr<BodyPart>>& parts, int amount) {
    // Priority 1: Vital organs
    for (auto& part : parts) {
        if (part->is_vital && part->current_hitpoints < part->max_hitpoints) {
            part->heal(amount);
            return true;
        }
        if (tryHealRecursive(part->internal_parts, amount)) return true;
    }
    
    // Priority 2: Non-vital parts (only if no vital parts needed healing in this branch)
    // The loop above returns if it finds a vital part. If we are here, no vital parts in this branch need healing.
    // However, this is depth-first. To strictly prioritize vital parts globally, we should run two passes.
    return false;
}

bool tryHealNonVitalRecursive(std::vector<std::unique_ptr<BodyPart>>& parts, int amount) {
     for (auto& part : parts) {
        if (!part->is_vital && part->current_hitpoints < part->max_hitpoints) {
            part->heal(amount);
            return true;
        }
        if (tryHealNonVitalRecursive(part->internal_parts, amount)) return true;
    }
    return false;
}

void PhysiologySystem::processHealing(AnatomyComponent* anatomy) {
    // Natural healing requires energy
    if (anatomy->stored_energy > 500.0f) {
        // Heal 1 point per turn? Maybe too fast. 1 point every 10 turns?
        // Let's stick to 1 point per turn but high energy cost for now to test.
        int healCost = 10;
        int healAmount = 1;

        // Try heal vital
        if (tryHealRecursive(anatomy->body_parts, healAmount)) {
            anatomy->stored_energy -= healCost;
        } else {
            // If no vital parts need healing, heal others
             if (tryHealNonVitalRecursive(anatomy->body_parts, healAmount)) {
                anatomy->stored_energy -= healCost;
             }
        }
    }
}

// -----------------------------------------------------------------------------
// Helpers
// -----------------------------------------------------------------------------
float findOrganEfficiency(const std::vector<std::unique_ptr<BodyPart>>& parts, Organ::Type type) {
    for (const auto& part : parts) {
        // Check if this part is an organ
        // We need RTTI or a flag. Currently Organ inherits BodyPart. 
        // We can't easily cast without RTTI dynamic_cast, or we assume name matches or add a type field to base.
        // Base BodyPart doesn't have type.
        // But we added specific classes.
        // Hack: Check name for now or use dynamic_cast if enabled.
        // Safe C++ in this engine: dynamic_cast is likely fine if classes are polymorphic (they are).
        
        const auto* organ = dynamic_cast<const Organ*>(part.get());
        if (organ && organ->organ_type == type) {
            return organ->efficiency;
        }

        float childEff = findOrganEfficiency(part->internal_parts, type);
        if (childEff >= 0.0f) return childEff; // Found it
    }
    return -1.0f; // Not found
}

float PhysiologySystem::calculateOrganEfficiency(AnatomyComponent* anatomy, Organ::Type type) {
    float eff = findOrganEfficiency(anatomy->body_parts, type);
    if (eff < 0.0f) return 1.0f; // Assume perfect if organ missing (or not modeled)
    return eff;
}