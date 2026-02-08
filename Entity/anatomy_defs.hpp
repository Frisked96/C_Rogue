#pragma once

enum class BodyPartType { GENERIC, LIMB, ORGAN };
enum class LimbType { ARM, LEG, HEAD, TAIL, NONE };
enum class OrganType { HEART, LUNG, BRAIN, STOMACH, LIVER, KIDNEY, NONE };

struct PhysiologyConfig {
    bool has_blood = true;
    bool needs_oxygen = true;
    bool has_metabolism = true;
    bool feels_pain = true;
    bool can_bleed = true;
    bool has_nervous_system = true;
    float base_metabolic_rate = 0.5f;
    float max_blood_volume = 5.0f;
};
