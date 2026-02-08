#pragma once
#include <string>
#include <vector>
#include "anatomy_defs.hpp"

// Blueprint for a single body part in a template
struct BodyTemplatePart {
    std::string name;
    BodyPartType type = BodyPartType::GENERIC;
    LimbType limb_type = LimbType::NONE;
    OrganType organ_type = OrganType::NONE;

    // Base stats
    int max_hp = 10;
    bool is_vital = false; // "Vital" means destroying this part kills the entity regardless of config? Or config overrides?
                           // The user said "lung is not a vital organ anymore" if needs_oxygen is false.
                           // So this flag likely means "Structurally Vital" (like a head for a human, or a CPU for a robot).
    int armor = 0;
    float width = 0.5f;
    float height = 0.5f;

    // Hierarchy
    std::string parent_name; // "ROOT" if no parent
    float relative_x = 0.0f;
    float relative_y = 0.0f;

    // Specifics
    std::vector<std::string> tags; // BioTags
    float strength = 0.0f;
    float dexterity = 0.0f;
    float efficiency = 1.0f;

    BodyTemplatePart() = default;

    // Convenience constructor
    BodyTemplatePart(const std::string& n, const std::string& p, int hp, BodyPartType t = BodyPartType::GENERIC)
        : name(n), parent_name(p), max_hp(hp), type(t) {}
};

// Complete blueprint for an entity's anatomy
struct BodyTemplate {
    std::string template_name;
    PhysiologyConfig physiology;
    std::vector<BodyTemplatePart> parts;

    void addPart(const BodyTemplatePart& part) {
        parts.push_back(part);
    }
};
