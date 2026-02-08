#include "physiology_system.hpp"
#include "anatomy_components.hpp"
#include "biological_tags.hpp"
#include "components.hpp"
#include "entity.hpp"
#include "entity_manager.hpp"
#include <algorithm>
#include <cmath>
#include <iostream>

void PhysiologySystem::processEntity(Entity *entity, AnatomyComponent *anatomy,
                                     HealthComponent *health) {
  processCirculation(anatomy, health);
  processRespiration(anatomy);
  processMetabolism(anatomy, health);
  processPain(anatomy); // Updates base pain
  processStress(
      anatomy); // Updates stress/adrenaline and modifies effective pain
  processHealing(anatomy);
}

// -----------------------------------------------------------------------------
// Circulation & Bleeding
// -----------------------------------------------------------------------------

void PhysiologySystem::processCirculation(AnatomyComponent *anatomy,
                                          HealthComponent *health) {
  float totalBleeding = 0.0f;
  for (auto &part : anatomy->body_parts) {
    totalBleeding += part.bleeding_intensity;

    // Clotting logic: 10% chance to reduce bleeding
    if (part.bleeding_intensity > 0) {
      if ((rand() % 100) < 10) {
        part.bleeding_intensity = std::max(0, part.bleeding_intensity - 1);
      }
    }

    // Check Arterial Integrity - if severed, massive bleeding
    if (part.arterial_integrity < 0.2f) {
      // Force bleeding if artery is severed
      part.bleeding_intensity = std::max(part.bleeding_intensity, 5);
    }
  }

  // Blood loss
  // Arbitrary scale: 1 intensity = 0.01 liters per turn?
  float bloodLoss = totalBleeding * 0.05f;
  anatomy->blood_volume -= bloodLoss;
  if (anatomy->blood_volume < 0.0f)
    anatomy->blood_volume = 0.0f;

  // Direct health damage from massive blood loss (Hypovolemic shock)
  float bloodRatio = anatomy->blood_volume / anatomy->max_blood_volume;
  if (bloodRatio < 0.5f) {
    // Taking damage due to blood loss
    health->takeDamage(1);
  }

  // Heart efficiency affects circulation (and thus can indirectly affect
  // oxygen) Replaced OrganType::HEART with BioTags::CIRCULATION
  float heartEff = calculateFunctionEfficiency(anatomy, BioTags::CIRCULATION);

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
void PhysiologySystem::processRespiration(AnatomyComponent *anatomy) {
  // Replaced OrganType::LUNG with BioTags::RESPIRATION
  float lungEff = calculateFunctionEfficiency(anatomy, BioTags::RESPIRATION);
  float bloodRatio = anatomy->blood_volume / anatomy->max_blood_volume;

  // Base oxygen consumption
  float oxygenChange = -2.0f; // Consume 2% per turn

  // Recovery if breathing and have blood
  if (lungEff > 0.5f && bloodRatio > 0.4f) {
    oxygenChange += 5.0f * lungEff * bloodRatio;
  }

  anatomy->oxygen_saturation += oxygenChange;

  // Clamp
  if (anatomy->oxygen_saturation > 100.0f)
    anatomy->oxygen_saturation = 100.0f;
  if (anatomy->oxygen_saturation < 0.0f)
    anatomy->oxygen_saturation = 0.0f;

  // Hypoxia
  if (anatomy->oxygen_saturation < 30.0f) {
    // Hypoxia damage to vital organs would be cool, but for now simple global
    // damage via health component Or we could damage the Brain specifically?
    // Finding Brain via NEURAL tag
    bool foundBrain = false;
    for (auto &part : anatomy->body_parts) {
      if (part.hasTag(BioTags::NEURAL)) {
        part.takeDamage(1);
        foundBrain = true;
        // Don't break, might have distributed neural system
      }
    }

    // Fallback if no brain found but hypoxic
    if (!foundBrain) {
      // Generic system failure
      anatomy->stress_level += 5.0f;
    }
  }
}

// -----------------------------------------------------------------------------
// Metabolism
// -----------------------------------------------------------------------------
void PhysiologySystem::processMetabolism(AnatomyComponent *anatomy,
                                         HealthComponent *health) {
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
void PhysiologySystem::processPain(AnatomyComponent *anatomy) {
  float totalPain = 0.0f;
  for (const auto &part : anatomy->body_parts) {
    if (part.max_hitpoints > 0) {
      float damageRatio = 1.0f - (static_cast<float>(part.current_hitpoints) /
                                  part.max_hitpoints);
      // Add to total
      totalPain += (damageRatio * 10.0f);
    }
  }
  anatomy->accumulated_pain = totalPain;
}

// -----------------------------------------------------------------------------
// Stress & Adrenaline
// -----------------------------------------------------------------------------
void PhysiologySystem::processStress(AnatomyComponent *anatomy) {
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
    if (anatomy->adrenaline_level < 0.0f)
      anatomy->adrenaline_level = 0.0f;
  }

  // 2. Stress Mechanics
  // Pain causes stress
  if (anatomy->accumulated_pain > 5.0f) {
    anatomy->stress_level += anatomy->accumulated_pain * 0.01f;
  }

  // Decay Stress (slowly, requires safety/rest normally)
  if (anatomy->stress_level > 0.0f) {
    anatomy->stress_level -= 0.1f;
    if (anatomy->stress_level < 0.0f)
      anatomy->stress_level = 0.0f;
  }

  // Cap
  if (anatomy->stress_level > 100.0f)
    anatomy->stress_level = 100.0f;
  if (anatomy->adrenaline_level > 100.0f)
    anatomy->adrenaline_level = 100.0f;

  // 3. Adrenaline Masking Pain
  // High adrenaline masks pain
  if (anatomy->adrenaline_level > 10.0f) {
    float maskAmount = anatomy->adrenaline_level * 0.5f;
    anatomy->accumulated_pain -= maskAmount;
    if (anatomy->accumulated_pain < 0.0f)
      anatomy->accumulated_pain = 0.0f;
  }
}

// -----------------------------------------------------------------------------
// Healing
// -----------------------------------------------------------------------------
void PhysiologySystem::processHealing(AnatomyComponent *anatomy) {
  // Natural healing requires energy
  if (anatomy->stored_energy > 500.0f) {
    int healCost = 10;
    int healAmount = 1;
    bool healed = false;

    // Priority 1: Vital organs
    for (auto &part : anatomy->body_parts) {
      if (part.is_vital && part.current_hitpoints < part.max_hitpoints) {
        part.heal(healAmount);
        healed = true;
        break;
      }
    }

    // Priority 2: Non-vital parts
    if (!healed) {
      for (auto &part : anatomy->body_parts) {
        if (!part.is_vital && part.current_hitpoints < part.max_hitpoints) {
          part.heal(healAmount);
          healed = true;
          break;
        }
      }
    }

    if (healed) {
      anatomy->stored_energy -= healCost;
    }
  }
}

// -----------------------------------------------------------------------------
// Helpers
// -----------------------------------------------------------------------------
float PhysiologySystem::calculateFunctionEfficiency(AnatomyComponent *anatomy,
                                                    const std::string &tag) {
  float totalEfficiency = 0.0f;
  int count = 0;

  for (const auto &part : anatomy->body_parts) {
    if (part.hasTag(tag)) {
      totalEfficiency += part.efficiency;
      count++;
    }
  }

  if (count == 0)
    return 1.0f; // Assume perfect if system not present (abstracted away)

  // Average efficiency of all parts contributing to this function
  return totalEfficiency / count;
}

bool PhysiologySystem::shouldKeepActive(AnatomyComponent *anatomy,
                                        HealthComponent *health) {
  if (!health->is_alive)
    return false;

  // Check Bleeding
  for (const auto &part : anatomy->body_parts) {
    if (part.bleeding_intensity > 0)
      return true;
  }

  // Check Pain & Stress
  if (anatomy->accumulated_pain > 5.0f)
    return true;
  if (anatomy->stress_level > 10.0f)
    return true;

  // Check Health (Active Healing needed?)
  bool lowHealth = health->current_health < health->max_health;
  bool canHeal = anatomy->stored_energy > 500.0f && lowHealth;

  if (canHeal)
    return true;

  return false;
}
