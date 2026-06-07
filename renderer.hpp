#pragma once

#include <iostream>
#include <vector>
#include <string>
#include "game_map.hpp"
#include "Entity/entity_manager.hpp"

using namespace std;

class Terminal_renderer {
private:
  int height;
  int width;
  vector<vector<char>> view_grid;

public:
  Terminal_renderer(int w, int h);
  void clear_screen();
  void clear_buffer();
  void draw();
  void set_tile(int x, int y, char c);
  void render_map(const Game_map &map, int z);
  void render_entities(EntityManager &entityManager, int z);
  void draw_ui(const Entity *player);
};
