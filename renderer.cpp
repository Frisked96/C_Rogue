#include "renderer.hpp"

Terminal_renderer::Terminal_renderer(int w, int h)
    : height(h), width(w), view_grid(h, vector<Cell>(w, {' ', 7, 0})) {}

void Terminal_renderer::clear_screen() {
  // Move cursor to 1,1
  std::cout << "\033[1;1H";
}

void Terminal_renderer::clear_buffer() {
  for (int y = 0; y < height; ++y) {
    for (int x = 0; x < width; ++x) {
      view_grid[y][x] = {' ', 7, 0};
    }
  }
}

void Terminal_renderer::draw() {
  std::string output;
  int current_fg = -1;
  int current_bg = -1;
  for (int y = 0; y < height; ++y) {
    for (int x = 0; x < width; ++x) {
      const Cell& cell = view_grid[y][x];
      if (cell.fg != current_fg) {
          output += "\033[38;5;" + std::to_string(cell.fg) + "m";
          current_fg = cell.fg;
      }
      if (cell.bg != current_bg) {
          if (cell.bg == 0) {
              output += "\033[49m"; // Default background
          } else {
              output += "\033[48;5;" + std::to_string(cell.bg) + "m";
          }
          current_bg = cell.bg;
      }
      output += cell.glyph;
    }
    output += "\033[0m\n";
    current_fg = -1;
    current_bg = -1;
  }
  std::cout << output;
}

void Terminal_renderer::set_tile(int x, int y, char c, int fg) {
    if (x >= 0 && x < width && y >= 0 && y < height) {
        view_grid[y][x] = {c, fg, 0};
    }
}

// Overload or helper for setting bg
void set_tile_bg(vector<vector<Terminal_renderer::Cell>>& grid, int x, int y, int bg) {
    if (x >= 0 && x < (int)grid[0].size() && y >= 0 && y < (int)grid.size()) {
        grid[y][x].bg = bg;
    }
}

void Terminal_renderer::render_map(const Game_map &map, int z, int cam_x, int cam_y) {
  int start_x = cam_x - width / 2;
  int start_y = cam_y - height / 2;

  for (int vy = 0; vy < height; ++vy) {
    for (int vx = 0; vx < width; ++vx) {
      int mx = start_x + vx;
      int my = start_y + vy;

      if (!map.is_in_bounds(mx, my, z)) {
          set_tile(vx, vy, ' ', 0);
          continue;
      }

      if (!map.is_explored(mx, my, z)) {
          set_tile(vx, vy, ' ', 0);
          continue;
      }

      int cz = z;
      const Tile* t = &map.get_tile(mx, my, cz);
      
      // Look down through air to find the first solid/non-air tile
      while (cz > 0 && t->material == MaterialType::AIR) {
          cz--;
          t = &map.get_tile(mx, my, cz);
      }
      
      int color = t->mat().fg_color;
      int bg_color = 0;
      bool visible = map.is_visible(mx, my, z); // Use player level visibility for the column

      if (visible) {
          // Highlight tiles on the same level as the player (except water)
          if (cz == z && t->material != MaterialType::WATER_FRESH) {
              bg_color = 236; // Dark gray background for current elevation
          }

          // Depth Dimming: If the ground is below the player, make it dimmer
          int depth = z - cz;
          if (depth > 0) {
              if (color >= 8 && color <= 15) {
                  color -= 8;
              }
              if (depth > 2) {
                  color = std::max(232, 255 - (depth * 2));
              }
          }
      } else {
          // Explored but not currently visible: Dim Gray
          color = 237; 
      }
      
      view_grid[vy][vx] = {t->get_glyph(), color, bg_color};
    }
  }
}

void Terminal_renderer::render_entities(EntityManager &entityManager, const Game_map &map, int z, int cam_x, int cam_y) {
  int start_x = cam_x - width / 2;
  int start_y = cam_y - height / 2;

  auto entities = entityManager.get_all_active();
  for (auto *entity : entities) {
    if (entity->state.z == z) {
      if (map.is_visible(entity->state.x, entity->state.y, entity->state.z) || entity->type == EntityType::PLAYER) {
          int vx = entity->state.x - start_x;
          int vy = entity->state.y - start_y;
          if (vx >= 0 && vx < width && vy >= 0 && vy < height) {
              view_grid[vy][vx].glyph = entity->props().glyph;
              view_grid[vy][vx].fg = entity->props().fg_color | 8;
          }
      }
    }
  }
}

void Terminal_renderer::draw_ui(const Entity *player, const Game_map &map, const std::string& msg) {
  // Print stats/help (pad with spaces to overwrite old text)
  if (player) {
      int px = player->state.x;
      int py = player->state.y;
      int pz = player->state.z;

      std::string surface_name = "Air";
      if (pz > 0) {
          surface_name = map.get_tile(px, py, pz - 1).mat().name;
      }
      
      bool ceiling = (pz < map.get_depth() - 1) && map.get_tile(px, py, pz + 1).material != MaterialType::AIR;

      std::cout << "\033[1;37m" << player->props().name 
                << " | Alt: " << pz << "m"
                << " | Standing on: " << surface_name << "          \n";
      
      std::cout << "Hunger: " << (int)(player->state.hunger * 100) << "% "
                << "| Thirst: " << (int)(player->state.thirst * 100) << "% "
                << "| Site: " << (ceiling ? "\033[33mUnderground\033[37m" : "\033[36mOpen Sky\033[37m") << "    \n";

      // Message Log
      std::cout << "\033[1;33mLog: " << msg << "\033[0m                                          \n";

      // Simple cardinal surroundings (Compass)
      auto get_alt_diff = [&](int dx, int dy) -> std::string {
          int nx = px + dx;
          int ny = py + dy;
          if (!map.is_in_bounds(nx, ny, pz)) return "???";
          if (!map.is_visible(nx, ny, pz)) return "???"; // Hide unknown altitude
          int nz = pz;
          // Find surface at nx, ny
          while (nz > 0 && map.get_tile(nx, ny, nz).material == MaterialType::AIR) nz--;
          int diff = nz - (pz - 1); // diff from ground under player
          if (diff == 0) return "=";
          return (diff > 0 ? "+" : "") + std::to_string(diff);
      };

      std::cout << "Near: [N:" << get_alt_diff(0, -1) << "] [S:" << get_alt_diff(0, 1) 
                << "] [W:" << get_alt_diff(-1, 0) << "] [E:" << get_alt_diff(1, 0) << "]    \n";
  }
  std::cout << "\033[0;32m--------------------------------------------------------\033[0m              \n";
}
