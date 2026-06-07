#include "renderer.hpp"
#include "Entity/components.hpp"

Terminal_renderer::Terminal_renderer(int w, int h)
    : width(w), height(h), view_grid(h, vector<char>(w, ' ')) {}

void Terminal_renderer::clear_screen() {
  // Move cursor to 1,1
  std::cout << "\033[1;1H";
}

void Terminal_renderer::clear_buffer() {
  for (int y = 0; y < height; ++y) {
    for (int x = 0; x < width; ++x) {
      view_grid[y][x] = ' ';
    }
  }
}

void Terminal_renderer::draw() {
  std::string output;
  for (int y = 0; y < height; ++y) {
    for (int x = 0; x < width; ++x) {
      output += view_grid[y][x];
    }
    output += '\n';
  }
  std::cout << output;
}

void Terminal_renderer::set_tile(int x, int y, char c) {
  if (x >= 0 && x < width && y >= 0 && y < height) {
    view_grid[y][x] = c;
  }
}

void Terminal_renderer::render_map(const Game_map &map, int z) {
  for (int y = 0; y < map.get_height(); ++y) {
    for (int x = 0; x < map.get_width(); ++x) {
      const auto &t = map.get_tile(x, y, z);
      set_tile(x, y, t.get_glyph());
    }
  }
}

void Terminal_renderer::render_entities(EntityManager &entityManager, int z) {
  auto entities = entityManager.getAllEntities();
  for (auto *entity : entities) {
    if (entity->hasComponent<PositionComponent>() &&
        entity->hasComponent<RenderComponent>()) {
      auto pos = entity->getComponent<PositionComponent>();
      if (pos->z == z) {
        auto render = entity->getComponent<RenderComponent>();
        set_tile(pos->x, pos->y, render->glyph);
      }
    }
  }
}

void Terminal_renderer::draw_ui(const Entity *player) {
  // Print stats/help (pad with spaces to overwrite old text)
  if (player->hasComponent<PositionComponent>() && player->hasComponent<NameComponent>()) {
      auto pos = player->getComponent<PositionComponent>();
      auto name = player->getComponent<NameComponent>();
      std::cout << "Name: " << name->name << " | Pos: (" << pos->x << "," << pos->y << "," << pos->z << ")    \n";
  }
  std::cout << "Use WASD to move, Q to quit.              \n";
}
