#pragma once

namespace BioTags {
// Vital Functions
// If the aggregate efficiency of these drops to 0, the entity dies immediately.
constexpr const char *CIRCULATION =
    "Circulation"; // Required for Oxygen transport
constexpr const char *RESPIRATION = "Respiration"; // Required for Oxygen intake
constexpr const char *NEURAL = "Neural"; // Required for Action/Consciousness

// Sustainment Functions
// Loss leads to Sickness, Toxins, or Starvation over time.
constexpr const char *DIGESTION = "Digestion"; // Converts Food -> Energy
constexpr const char *FILTRATION =
    "Filtration"; // Removes Toxins/Drug accumulation
constexpr const char *ENDOCRINE =
    "Endocrine"; // Regulates Stress/Adrenaline recovery

// Capability Functions
// Determines what the entity can DO.
constexpr const char *MOTILITY = "Motility";         // Walking/Running speed
constexpr const char *MANIPULATION = "Manipulation"; // Weapon handling/Leverage
constexpr const char *SIGHT = "Sight";               // Vision range/Accuracy

// Exotic / Mutation Functions
// Special tags for non-standard biology.
constexpr const char *REGENERATION = "Regeneration"; // Passive HP recovery
constexpr const char *THERMAL_GEN =
    "ThermalGen"; // Body heat generation (resistance to cold)
constexpr const char *TOXIN_GEN = "ToxinGen"; // Natural poison production
} // namespace BioTags
