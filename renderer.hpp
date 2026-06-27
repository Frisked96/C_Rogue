#pragma once

#include "Entity/entity_manager.hpp"
#include "game_map.hpp"
#include <iostream>
#include <string>
#include <vector>

using namespace std;

class Terminal_renderer {
public:
  struct Cell {
    char glyph;
    int fg;
    int bg;
  };

private:
  int height;
  int width;
  vector<vector<Cell>> view_grid;
  bool debug_mode = false;

  void render_map_debug(const Game_map &map, int z, int cam_x, int cam_y);
  void draw_ui_debug(const Entity *player, const Game_map &map,
                     const std::string &msg);

public:
  Terminal_renderer(int w, int h);
  void clear_screen();
  void clear_buffer();
  void draw();
  void set_tile(int x, int y, char c, int fg = 7);
  void render_map(const Game_map &map, int z, int cam_x, int cam_y);
  void render_entities(EntityManager &entityManager, const Game_map &map, int z,
                       int cam_x, int cam_y);
  void draw_ui(const Entity *player, const Game_map &map,
               const std::string &msg);
  void toggle_debug_mode();
  bool is_debug_mode() const;
};
