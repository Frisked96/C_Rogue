#pragma once
#include "render_types.hpp"
#include "../Entity/entity_manager.hpp"
#include "../Object/object_manager.hpp"
#include "../game_map.hpp"

class EntityRenderer {
public:
    void render(RenderPlane& plane, EntityManager& entityManager,
                ObjectManager* objManager, const Game_map& map,
                int z, int cam_x, int cam_y);
};
