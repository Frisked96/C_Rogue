#pragma once
#include "anatomy_part_def.hpp"

namespace Eye {
    inline BodyTemplatePart createHuman(const std::string& name = "Eye") {
        BodyTemplatePart p(name, "", 5, BodyPartType::ORGAN);
        p.organ_type = OrganType::EYE;
        p.width = 0.03f;
        p.height = 0.03f;
        p.depth = 0.03f;
        p.is_vital = false;
        p.tags.push_back("Sight");
        return p;
    }

    inline BodyTemplatePart createInsect(const std::string& name = "Insect Eye") {
        BodyTemplatePart p(name, "", 2, BodyPartType::ORGAN);
        p.organ_type = OrganType::EYE;
        p.width = 0.01f;
        p.height = 0.01f;
        p.depth = 0.01f;
        p.tags.push_back("Sight");
        p.tags.push_back("Compound");
        return p;
    }

    inline BodyTemplatePart createMechanical(const std::string& name = "Cybernetic Eye") {
        BodyTemplatePart p(name, "", 10, BodyPartType::ORGAN);
        p.organ_type = OrganType::EYE;
        p.width = 0.03f;
        p.height = 0.03f;
        p.depth = 0.03f;
        p.is_vital = false;
        p.armor = 1;
        p.tags.push_back("Sight");
        p.tags.push_back("Mechanical");
        p.tags.push_back("Infrared");
        return p;
    }
}
