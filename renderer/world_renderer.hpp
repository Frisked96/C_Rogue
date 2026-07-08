#pragma once
#include "render_types.hpp"
#include "../game_map.hpp"

class WorldRenderer {
public:
    void render(RenderPlane& plane, const Game_map& map, int z, int cam_x, int cam_y);
    void render_debug(RenderPlane& plane, const Game_map& map, int z, int cam_x, int cam_y);
};
