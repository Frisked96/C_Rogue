#pragma once
#include "anatomy_part_def.hpp"

namespace Brain {
    inline BodyTemplatePart createHuman(const std::string& name = "Brain") {
        BodyTemplatePart p(name, "", 5, BodyPartType::ORGAN);
        p.organ_type = OrganType::BRAIN;
        p.width = 0.1f;
        p.height = 0.1f;
        p.depth = 0.1f;
        p.is_vital = true;
        p.tags.push_back("Neural");
        return p;
    }

    inline BodyTemplatePart createPositronic(const std::string& name = "Positronic Brain") {
        BodyTemplatePart p(name, "", 15, BodyPartType::ORGAN);
        p.organ_type = OrganType::BRAIN;
        p.width = 0.1f;
        p.height = 0.1f;
        p.depth = 0.1f;
        p.is_vital = true;
        p.armor = 3;
        p.tags.push_back("Neural");
        p.tags.push_back("Mechanical");
        return p;
    }
}
