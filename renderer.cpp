#include "renderer.hpp"
#include "Entity/components.hpp"
#include <iostream>
#include <vector>

using namespace std;

Terminal_renderer::Terminal_renderer(int w, int h)
    : height(h), width(w), view_grid(h, vector<char>(w, '.')) {}

void Terminal_renderer::clear_screen() {
  // Move cursor to top-left and hide cursor
  std::cout << "\033[H\033[?25l";
}

void Terminal_renderer::clear_buffer() {
  for (auto &row : view_grid) {
    fill(row.begin(), row.end(), '.');
  }
}

void Terminal_renderer::draw() {
  std::string output = "";
  for (auto &row : view_grid) {
    for (char c : row)
      output += c;
    output += "\n";
  }
  std::cout << output;
}

void Terminal_renderer::set_tile(int x, int y, char c) {
  if (y >= 0 && y < height && x >= 0 && x < width) {
    view_grid[y][x] = c;
  }
}

void Terminal_renderer::render_map(const Game_map &map) {
  for (int y = 0; y < map.get_height(); ++y) {
    for (int x = 0; x < map.get_width(); ++x) {
      Tile t = map.get_tile(x, y);
      set_tile(x, y, t.glyph);
    }
  }
}

void Terminal_renderer::render_entities(EntityManager &entityManager) {
  auto entities = entityManager.getAllEntities();
  for (auto *entity : entities) {
    if (entity->hasComponent<PositionComponent>() &&
        entity->hasComponent<RenderComponent>()) {
      auto pos = entity->getComponent<PositionComponent>();
      auto render = entity->getComponent<RenderComponent>();
      set_tile(pos->x, pos->y, render->glyph);
    }
  }
}

void Terminal_renderer::draw_ui(const Entity *player) {
  // Print stats/help (pad with spaces to overwrite old text)
  if (player && player->hasComponent<PositionComponent>()) {
    auto pos = player->getComponent<PositionComponent>();
    std::cout << "Player: (" << pos->x << ", " << pos->y << ")               \n";
  }
  std::cout << "Controls: WASD to move, Q to quit.                         \n";
}
