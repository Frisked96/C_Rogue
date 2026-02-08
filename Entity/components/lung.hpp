#pragma once
#include "anatomy_part_def.hpp"

namespace Lung {
    inline BodyTemplatePart createHuman(const std::string& name = "Lung") {
        BodyTemplatePart p(name, "", 15, BodyPartType::ORGAN);
        p.organ_type = OrganType::LUNG;
        p.width = 0.2f;
        p.height = 0.2f;
        p.depth = 0.2f;
        p.is_vital = true;
        p.tags.push_back("Respiration");
        return p;
    }

    inline BodyTemplatePart createMechanical(const std::string& name = "Respirator Implant") {
        BodyTemplatePart p(name, "", 20, BodyPartType::ORGAN);
        p.organ_type = OrganType::LUNG;
        p.width = 0.15f;
        p.height = 0.15f;
        p.depth = 0.15f;
        p.is_vital = true;
        p.armor = 1;
        p.tags.push_back("Respiration");
        p.tags.push_back("Mechanical");
        return p;
    }
}
