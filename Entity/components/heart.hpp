#pragma once
#include "anatomy_part_def.hpp"

namespace Heart {
    inline BodyTemplatePart createBiological(const std::string& name = "Heart") {
        BodyTemplatePart p(name, "", 10, BodyPartType::ORGAN);
        p.organ_type = OrganType::HEART;
        p.width = 0.15f;
        p.height = 0.15f;
        p.depth = 0.15f;
        p.is_vital = true;
        p.tags.push_back("Circulation");
        return p;
    }

    inline BodyTemplatePart createMechanical(const std::string& name = "Mechanical Heart") {
        BodyTemplatePart p(name, "", 25, BodyPartType::ORGAN);
        p.organ_type = OrganType::HEART;
        p.width = 0.15f;
        p.height = 0.15f;
        p.depth = 0.15f;
        p.is_vital = true;
        p.armor = 2;
        p.tags.push_back("Circulation");
        p.tags.push_back("Mechanical");
        return p;
    }
}
