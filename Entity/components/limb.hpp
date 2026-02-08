#pragma once
#include "anatomy_part_def.hpp"

class Limb {
public:
    BodyTemplatePart self;
    std::vector<BodyTemplatePart> internal_parts;
    std::vector<std::shared_ptr<Limb>> children;

    Limb(const std::string& name, LimbType type, int hp, float w, float h, float d) {
        self.name = name;
        self.type = BodyPartType::LIMB;
        self.limb_type = type;
        self.max_hp = hp;
        self.width = w;
        self.height = h;
        self.depth = d;
    }

    // Add an organ directly inside this limb
    void addInternalPart(BodyTemplatePart part, float rx = 0, float ry = 0, float rz = 0) {
        part.relative_x = rx;
        part.relative_y = ry;
        part.relative_z = rz;
        internal_parts.push_back(part);
    }

    // Attach a child limb to this limb
    void addChildLimb(std::shared_ptr<Limb> child, float rx = 0, float ry = 0, float rz = 0) {
        if (child) {
            child->self.relative_x = rx;
            child->self.relative_y = ry;
            child->self.relative_z = rz;
            children.push_back(child);
        }
    }

    // Flatten into template
    // returns the index of 'self' in the parts list
    int flatten(BodyTemplate& t, int parent_idx = -1) {
        int my_idx = (int)t.parts.size();
        self.parent_index = parent_idx;

        if (parent_idx == -1) {
             self.parent_name = "ROOT";
        }

        t.addPart(self);

        // Add internal parts (organs)
        for (auto& p : internal_parts) {
            p.parent_index = my_idx;
            // We clear parent_name to ensure index is preferred, or set it to self.name for debug
            p.parent_name = self.name;
            t.addPart(p);
        }

        // Add child limbs
        for (auto& child : children) {
            child->flatten(t, my_idx);
        }

        return my_idx;
    }

    // Static helper to create a Limb pointer easily
    static std::shared_ptr<Limb> create(const std::string& name, LimbType type, int hp, float w, float h, float d) {
        return std::make_shared<Limb>(name, type, hp, w, h, d);
    }
};
