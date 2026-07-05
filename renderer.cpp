#include "renderer.hpp"
#include <algorithm>
#include <iomanip>
#include <sstream>

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
      const Cell &cell = view_grid[y][x];
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
void set_tile_bg(vector<vector<Terminal_renderer::Cell>> &grid, int x, int y,
                 int bg) {
  if (x >= 0 && x < (int)grid[0].size() && y >= 0 && y < (int)grid.size()) {
    grid[y][x].bg = bg;
  }
}

void Terminal_renderer::render_map(const Game_map &map, int z, int cam_x,
                                   int cam_y) {
  if (debug_mode) {
    render_map_debug(map, z, cam_x, cam_y);
    return;
  }
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
      const Tile *t = &map.get_tile(mx, my, cz);

      // Look down through air to find the first solid or air-with-liquid/ice
      // tile
      while (cz > 0 && t->material == MaterialType::AIR &&
             t->state.liquid_volume <= 0.0f && t->state.frozen_volume <= 0.0f) {
        cz--;
        t = &map.get_tile(mx, my, cz);
      }

      int color = t->mat().fg_color;
      int bg_color = 0;
      char glyph = t->get_glyph();

      // --- Water / Ice rendering (with stacked-water fix) ---
      if (t->material == MaterialType::AIR &&
          (t->state.liquid_volume > 0.0f || t->state.frozen_volume > 0.0f)) {

        if (t->state.frozen_volume > 0.0f) {
          glyph = '*'; // Snow/Ice
          color = 15;  // White
        } else {
          // Accumulate the total water column depth from cz downward.
          // This fixes the bug where shallow water stacked over more water
          // tiles rendered as blank space.
          float total_water = t->state.liquid_volume;
          int water_bottom = cz;
          while (water_bottom > 0) {
            const Tile &wb = map.get_tile(mx, my, water_bottom - 1);
            if (wb.material == MaterialType::AIR &&
                wb.state.liquid_volume > 0.0f) {
              total_water += wb.state.liquid_volume;
              water_bottom--;
            } else {
              break;
            }
          }

          // Find the solid ground beneath the entire water column (for
          // shallow rendering and shoreline detection)
          int ground_z = water_bottom - 1;
          const Tile *ground = nullptr;
          if (ground_z >= 0) {
            ground = &map.get_tile(mx, my, ground_z);
            if (ground->material == MaterialType::AIR)
              ground = nullptr;
          }

          // Three-tier water depth rendering:
          //   shallow  (total < 0.4)  — show ground through tinted water
          //   medium   (0.4 .. 1.5)   — wavy surface
          //   deep     (> 1.5)        — dense water block
          if (total_water < 0.4f && ground) {
            // Shallow: ground glyph visible through water
            glyph = ground->get_glyph();
            color = ground->mat().fg_color;
            if (total_water > 0.15f) {
              bg_color = 24; // Teal Blue tint (visible)
            }
          } else if (total_water < 1.5f) {
            // Medium depth
            glyph = '~';
            color = 12;    // Light Blue
            bg_color = 24; // Teal Blue
          } else {
            // Deep water
            glyph = '~';
            color = 33;    // Blue glyph for deep water
            bg_color = 25; // Deep Blue background
          }

          // --- Shoreline detection ---
          // If this tile has water, check if any cardinal neighbour at the
          // same level is dry land.  If so, mark it as a shoreline tile
          // with a distinct glyph to clearly delineate water edges.
          if (total_water >= 0.15f) {
            bool is_shoreline = false;
            static const int dx[] = {0, 0, -1, 1};
            static const int dy[] = {-1, 1, 0, 0};
            for (int d = 0; d < 4; ++d) {
              int nx = mx + dx[d];
              int ny = my + dy[d];
              if (!map.is_in_bounds(nx, ny, z))
                continue;
              // Walk down at the neighbour column to find its surface
              int nz = z;
              const Tile *nt = &map.get_tile(nx, ny, nz);
              while (nz > 0 && nt->material == MaterialType::AIR &&
                     nt->state.liquid_volume <= 0.0f &&
                     nt->state.frozen_volume <= 0.0f) {
                nz--;
                nt = &map.get_tile(nx, ny, nz);
              }
              // Neighbour is dry land (solid, no water)
              if (nt->material != MaterialType::AIR) {
                is_shoreline = true;
                break;
              }
            }
            if (is_shoreline) {
              glyph = ',';   // Shoreline marker — small, unobtrusive
              color = 45;    // Cyan/Teal for coastal feel
              bg_color = 24; // Keep the water background
            }
          }
        }
      }

      bool visible = map.is_visible(
          mx, my, z); // Use player level visibility for the column

      if (visible) {
        // --- Elevation contour background coloring ---
        // Apply a subtle background gradient based on how far below the
        // player this tile's surface is, so elevation changes are visible
        // even on dry land. Water tiles keep their own bg_color.
        if (bg_color == 0) {
          int depth = z - cz;
          if (depth <= 0) {
            // At or above player level — wall / ledge that blocks you
            bg_color = 88; // Red: unmistakably a raised obstacle
          } else if (depth == 1) {
            // Immediate floor level — where the player stands
            bg_color = 237; // Neutral Dark Gray: walkable ground
          } else if (depth == 2) {
            bg_color = 235; // Slightly darker — one step down
          } else if (depth == 3) {
            bg_color = 234; // Darker still — noticeable drop
          } else {
            // Deep pit / canyon — very dark
            bg_color = 233;
          }
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
        bg_color = 0;
      }

      view_grid[vy][vx] = {glyph, color, bg_color};
    }
  }
}

void Terminal_renderer::render_entities(EntityManager &entityManager,
                                        const Game_map &map, int z, int cam_x,
                                        int cam_y) {
  int start_x = cam_x - width / 2;
  int start_y = cam_y - height / 2;

  auto entities = entityManager.get_all_active();
  for (auto *entity : entities) {
    if (entity->state.z == z) {
      if (map.is_visible(entity->state.x, entity->state.y, entity->state.z) ||
          entity->type == EntityType::PLAYER) {
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

void Terminal_renderer::draw_ui(const Entity *player, const Game_map &map,
                                const std::string &msg) {
  if (debug_mode) {
    draw_ui_debug(player, map, msg);
    return;
  }
  // Print stats/help (pad with spaces to overwrite old text)
  if (player) {
    int px = player->state.x;
    int py = player->state.y;
    int pz = player->state.z;

    std::string surface_name = "Air";
    if (pz > 0) {
      surface_name = map.get_tile(px, py, pz - 1).mat().name;
    }

    bool ceiling = (pz < map.get_depth() - 1) &&
                   map.get_tile(px, py, pz + 1).material != MaterialType::AIR;

    std::cout << "\033[1;37m" << player->props().name << " | Alt: " << pz << "m"
              << " | Standing on: " << surface_name << "          \n";

    std::cout << "Hunger: " << (int)(player->state.hunger * 100) << "% "
              << "| Thirst: " << (int)(player->state.thirst * 100) << "% "
              << "| Site: "
              << (ceiling ? "\033[33mUnderground\033[37m"
                          : "\033[36mOpen Sky\033[37m")
              << "    \n";

    // Message Log
    std::cout << "\033[1;33mLog: " << msg
              << "\033[0m                                          \n";

    // Simple cardinal surroundings (Compass)
    auto get_alt_diff = [&](int dx, int dy) -> std::string {
      int nx = px + dx;
      int ny = py + dy;
      if (!map.is_in_bounds(nx, ny, pz))
        return "???";
      if (!map.is_visible(nx, ny, pz))
        return "???"; // Hide unknown altitude
      int nz = pz;
      // Find surface at nx, ny
      while (nz > 0 && map.get_tile(nx, ny, nz).material == MaterialType::AIR)
        nz--;
      int diff = nz - (pz - 1); // diff from ground under player
      if (diff == 0)
        return "=";
      return (diff > 0 ? "+" : "") + std::to_string(diff);
    };

    std::cout << "Near: [N:" << get_alt_diff(0, -1)
              << "] [S:" << get_alt_diff(0, 1) << "] [W:" << get_alt_diff(-1, 0)
              << "] [E:" << get_alt_diff(1, 0) << "]    \n";
  }
  std::cout << "\033[0;32m-----------------------------------------------------"
               "---\033[0m              \n";
}

// ============================================================
// Debug Mode
// ============================================================

void Terminal_renderer::toggle_debug_mode() { debug_mode = !debug_mode; }
bool Terminal_renderer::is_debug_mode() const { return debug_mode; }

// --- Debug Map Renderer ---
void Terminal_renderer::render_map_debug(const Game_map &map, int z, int cam_x,
                                         int cam_y) {
  static const int contour_bg[] = {232, 233, 234, 235, 236};
  int start_x = cam_x - width / 2;
  int start_y = cam_y - height / 2;

  for (int vy = 0; vy < height; ++vy) {
    for (int vx = 0; vx < width; ++vx) {
      int mx = start_x + vx;
      int my = start_y + vy;

      if (!map.is_in_bounds(mx, my, z)) {
        view_grid[vy][vx] = {' ', 0, 0};
        continue;
      }

      if (!map.is_explored(mx, my, z)) {
        view_grid[vy][vx] = {' ', 0, 0};
        continue;
      }

      // Check for water anywhere in this column (z down to ground)
      bool water_in_column = false;
      if (map.is_in_bounds(mx, my, z)) {
        for (int wz = z; wz >= 0; --wz) {
          const Tile &zt = map.get_tile(mx, my, wz);
          if (zt.material != MaterialType::AIR)
            break; // hit solid, stop
          if (zt.state.liquid_volume > 0.0f) {
            water_in_column = true;
            break;
          }
        }
      }

      // Raycast: look down through ALL air tiles (ignoring liquid/frozen)
      // until we find a solid tile
      int cz = z;
      const Tile *t = &map.get_tile(mx, my, cz);
      while (cz > 0 && t->material == MaterialType::AIR) {
        cz--;
        t = &map.get_tile(mx, my, cz);
      }

      int surface_z = cz;
      char glyph = '0' + (surface_z % 10);
      int fg = 232 + std::min(23, surface_z / 4);
      int bg = contour_bg[(surface_z / 5) % 5];

      bool visible = map.is_visible(mx, my, z);

      if (visible) {
        // Water presence indicator: teal background tint
        if (water_in_column) {
          bg = 24;
        }
      } else {
        // Explored but not currently visible: dim gray glyph
        fg = 237;
        bg = 0;
      }

      view_grid[vy][vx] = {glyph, fg, bg};
    }
  }
}

// --- Debug UI Panel ---
void Terminal_renderer::draw_ui_debug(const Entity *player, const Game_map &map,
                                      const std::string &msg) {
  if (!player) {
    std::cout << "\033[1;31m[DEBUG] No player entity\033[0m\n";
    return;
  }

  int px = player->state.x;
  int py = player->state.y;
  int pz = player->state.z;

  // Find surface_z below player
  int surface_z = pz;
  while (surface_z > 0 &&
         map.get_tile(px, py, surface_z).material == MaterialType::AIR) {
    surface_z--;
  }

  // --- Line 1: Header ---
  std::cout << "\033[1;32m[DEBUG MODE] Pos: (" << px << ", " << py << ", " << pz
            << ") | Surface Elev: " << surface_z << "m\033[0m    \n";

  // Helper to format floats
  auto ff = [](float v) -> std::string {
    std::ostringstream oss;
    oss << std::fixed << std::setprecision(2) << v;
    return oss.str();
  };

  // --- Lines 2-4: Tile player is INSIDE (z-level) ---
  if (map.is_in_bounds(px, py, pz)) {
    const Tile &t = map.get_tile(px, py, pz);
    const auto &m = t.mat();
    std::cout << "\033[36mINSIDE [" << pz << "]: \033[37m" << m.name
              << " | glyph:'" << m.glyph
              << "' solid:" << (m.is_solid ? "Y" : "N")
              << " opaque:" << (m.is_opaque ? "Y" : "N") << "    \n";
    std::cout << "\033[36m  Water: \033[37m" << ff(t.state.liquid_volume)
              << " \033[36mIce: \033[37m" << ff(t.state.frozen_volume)
              << " \033[36mTemp: \033[37m" << ff(t.state.temperature)
              << "K \033[36mCompact: \033[37m" << ff(t.state.compaction)
              << "    \n";
    std::cout << "\033[36m  Density: \033[37m" << ff(m.density_kgm3)
              << "kg/m3 \033[36mPoros: \033[37m" << ff(m.max_porosity)
              << " \033[36mPerm: \033[37m" << ff(m.permeability)
              << " \033[36mShear: \033[37m" << ff(m.shear_strength)
              << "kPa    \n";
  }

  // --- Lines 5-7: Tile player is STANDING ON (z-1) ---
  if (pz > 0 && map.is_in_bounds(px, py, pz - 1)) {
    const Tile &t = map.get_tile(px, py, pz - 1);
    const auto &m = t.mat();
    std::cout << "\033[36mON [" << (pz - 1) << "]: \033[37m" << m.name
              << " | glyph:'" << m.glyph
              << "' solid:" << (m.is_solid ? "Y" : "N")
              << " opaque:" << (m.is_opaque ? "Y" : "N") << "    \n";
    std::cout << "\033[36m  Water: \033[37m" << ff(t.state.liquid_volume)
              << " \033[36mIce: \033[37m" << ff(t.state.frozen_volume)
              << " \033[36mTemp: \033[37m" << ff(t.state.temperature)
              << "K \033[36mCompact: \033[37m" << ff(t.state.compaction)
              << "    \n";
    std::cout << "\033[36m  Density: \033[37m" << ff(m.density_kgm3)
              << "kg/m3 \033[36mPoros: \033[37m" << ff(m.max_porosity)
              << " \033[36mPerm: \033[37m" << ff(m.permeability)
              << " \033[36mShear: \033[37m" << ff(m.shear_strength)
              << "kPa    \n";
  }

  // --- Water Compass ---
  // Scans each cardinal direction. At each step, checks the full
  // vertical column from pz downward to find water at ANY z-level.
  static const int dx[] = {0, 0, -1, 1};
  static const int dy[] = {-1, 1, 0, 0};
  static const char *dir_names[] = {"N", "S", "W", "E"};
  int water_dist[4] = {-1, -1, -1, -1};
  int water_z[4] = {-1, -1, -1, -1};

  for (int d = 0; d < 4; ++d) {
    for (int step = 1; step <= 50; ++step) {
      int nx = px + dx[d] * step;
      int ny = py + dy[d] * step;
      bool found = false;
      // Scan from pz downward through air to find water
      for (int sz = pz; sz >= 0; --sz) {
        if (!map.is_in_bounds(nx, ny, sz))
          break;
        const Tile &nt = map.get_tile(nx, ny, sz);
        if (nt.material != MaterialType::AIR)
          break; // hit solid, stop
        if (nt.state.liquid_volume > 0.0f) {
          water_dist[d] = step;
          water_z[d] = sz;
          found = true;
          break;
        }
      }
      if (found)
        break;
    }
  }

  // Check if player is currently in water (any z at player column)
  bool in_water = false;
  int player_water_z = -1;
  for (int sz = pz; sz >= 0; --sz) {
    if (!map.is_in_bounds(px, py, sz))
      break;
    const Tile &pt = map.get_tile(px, py, sz);
    if (pt.material != MaterialType::AIR)
      break;
    if (pt.state.liquid_volume > 0.0f) {
      in_water = true;
      player_water_z = sz;
      break;
    }
  }

  std::cout << "\033[33m";
  if (in_water) {
    std::cout << "[IN WATER z:" << player_water_z << "] ";
  }
  std::cout << "Water:";
  for (int d = 0; d < 4; ++d) {
    std::cout << " [" << dir_names[d] << ":";
    if (water_dist[d] >= 0)
      std::cout << water_dist[d] << "@z" << water_z[d];
    else
      std::cout << "---";
    std::cout << "]";
  }

  // Find nearest
  int nearest = -1;
  int nearest_dist = 51;
  for (int d = 0; d < 4; ++d) {
    if (water_dist[d] >= 0 && water_dist[d] < nearest_dist) {
      nearest_dist = water_dist[d];
      nearest = d;
    }
  }
  std::cout << "  Nearest: " << (nearest >= 0 ? dir_names[nearest] : "NONE")
            << "\033[0m    \n";

  // --- Log ---
  std::cout << "\033[1;33mLog: " << msg
            << "\033[0m                                          \n";

  // --- Separator ---
  std::cout << "\033[0;32m[DEBUG]----------------------------------------------"
               "--------"
               "\033[0m    \n";
}
