#pragma once
#include "../Entity/entity_manager.hpp"
#include "../Object/object_manager.hpp"
#include "../game_map.hpp"
#include <string>

class UIRenderer {
public:
    // Renders UI text directly to stdout. The cursor should already be
    // positioned below the map grid before calling this.
    void render(const Entity* player, ObjectManager* objManager,
                const Game_map& map, const std::string& msg,
                bool debug_mode);
private:
    void render_normal(const Entity* player, ObjectManager* objManager,
                       const Game_map& map, const std::string& msg);
    void render_debug(const Entity* player, const Game_map& map,
                      const std::string& msg);
};
