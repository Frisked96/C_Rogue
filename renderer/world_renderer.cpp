#include "world_renderer.hpp"
#include <algorithm>

namespace {
struct ColumnInfo {
  int surface_z;      // first non‑air solid (or water/ice) below z0
  const Tile *tile;   // pointer to that tile
  float water_volume; // total water in column from z0 down to ground
  bool is_water;      // top layer is water or ice
  bool is_ice;        // top layer is ice
};

// Compute the surface column for (mx,my) starting from z0.
// Returns info about what would be rendered.
ColumnInfo compute_surface(const Game_map &map, int mx, int my, int z0) {
  ColumnInfo ci{};
  int cz = z0;
  const Tile *t = &map.get_tile(mx, my, cz);
  // look down through air / water / ice
  while (cz > 0 && t->material == MaterialType::AIR) {
    cz--;
    t = &map.get_tile(mx, my, cz);
  }
  ci.surface_z = cz;
  ci.tile = t;

  // Compute total water in column if top is water.
  if (t->material == MaterialType::AIR &&
      (t->state.liquid_volume > 0.0f || t->state.frozen_volume > 0.0f)) {
    ci.is_water = true;
    ci.is_ice = (t->state.frozen_volume > 0.0f);
    float total = t->state.liquid_volume;
    int wb = cz;
    while (wb > 0) {
      const Tile &wt = map.get_tile(mx, my, wb - 1);
      if (wt.material == MaterialType::AIR && wt.state.liquid_volume > 0.0f) {
        total += wt.state.liquid_volume;
        wb--;
      } else
        break;
    }
    ci.water_volume = total;
  }
  return ci;
}
} // namespace

void WorldRenderer::render(RenderPlane &plane, const Game_map &map, int z,
                           int cam_x, int cam_y) {
  int width = plane.width();
  int height = plane.height();
  int start_x = cam_x - width / 2;
  int start_y = cam_y - height / 2;

  for (int vy = 0; vy < height; ++vy) {
    for (int vx = 0; vx < width; ++vx) {
      int mx = start_x + vx;
      int my = start_y + vy;

      if (!map.is_in_bounds(mx, my, z)) {
        plane.set(vx, vy, ' ', 0, 0);
        continue;
      }

      // Compute surface column
      ColumnInfo col = compute_surface(map, mx, my, z);

      // Explored/visible check
      bool explored = false, visible = false;
      for (int ez = z; ez >= col.surface_z; --ez) {
        if (map.is_explored(mx, my, ez))
          explored = true;
        if (map.is_visible(mx, my, ez))
          visible = true;
        if (explored && visible)
          break;
      }
      if (!explored) {
        for (int ez = z + 1; ez < map.get_depth(); ++ez) {
          if (map.is_explored(mx, my, ez)) {
            col = compute_surface(map, mx, my, ez);
            explored = true;
            visible = map.is_visible(mx, my, ez);
            break;
          }
        }
      }
      if (!explored) {
        plane.set(vx, vy, ' ', 0, 0);
        continue;
      }

      char glyph = col.tile->get_glyph();
      int color = col.tile->mat().fg_color;
      int bg_color = 0;

      if (col.is_water) {
        if (col.is_ice) {
          glyph = '*';
          color = 15;
        } else {
          // Find ground below water for shallow rendering
          const Tile *ground = nullptr;
          int ground_z = col.surface_z - 1;
          if (ground_z >= 0) {
            ground = &map.get_tile(mx, my, ground_z);
            if (ground->material == MaterialType::AIR)
              ground = nullptr;
          }

          float total = col.water_volume;
          if (total < 0.4f && ground) {
            glyph = ground->get_glyph();
            color = ground->mat().fg_color;
            bg_color = (total > 0.15f) ? 17 : 16;
          } else if (total < 1.5f) {
            glyph = '~';
            color = 12;
            bg_color = 19;
          } else {
            glyph = '~';
            color = 33;
            bg_color = 25;
          }

          // Shoreline detection
          if (total >= 0.15f) {
            bool shore = false;
            static const int dx[] = {0, 0, -1, 1}, dy[] = {-1, 1, 0, 0};
            for (int d = 0; d < 4; ++d) {
              int nx = mx + dx[d], ny = my + dy[d];
              if (!map.is_in_bounds(nx, ny, z))
                continue;
              ColumnInfo nbr = compute_surface(map, nx, ny, z);
              if (nbr.tile->material != MaterialType::AIR) {
                shore = true;
                break;
              }
            }
            if (shore) {
              glyph = ',';
              color = 45;
              bg_color = 17;
            }
          }
        }
      }

      if (visible) {
        // Elevation background only if not already set by water.
        if (bg_color == 0) {
          int depth = z - col.surface_z;
          if (depth <= 0)
            bg_color = 52;
          else if (depth == 1)
            bg_color = 237;
          else if (depth == 2)
            bg_color = 235;
          else if (depth == 3)
            bg_color = 234;
          else
            bg_color = 233;
        }
        int depth = z - col.surface_z;
        if (depth > 0) {
          if (color >= 8 && color <= 15)
            color -= 8;
          if (depth > 2)
            color = std::max(232, 255 - (depth * 2));
        }
      } else {
        color = 237;
        bg_color = 0;
      }

      plane.set(vx, vy, glyph, color, bg_color);
    }
  }
}

// --- Debug Map Renderer ---
void WorldRenderer::render_debug(RenderPlane &plane, const Game_map &map, int z,
                                 int cam_x, int cam_y) {
  static const int contour_bg[] = {232, 233, 234, 235, 236};
  int width = plane.width();
  int height = plane.height();
  int start_x = cam_x - width / 2;
  int start_y = cam_y - height / 2;

  for (int vy = 0; vy < height; ++vy) {
    for (int vx = 0; vx < width; ++vx) {
      int mx = start_x + vx;
      int my = start_y + vy;

      if (!map.is_in_bounds(mx, my, z)) {
        plane.set(vx, vy, ' ', 0, 0);
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

      // Check explored across column
      bool explored = false;
      bool visible = false;
      for (int ez = z; ez >= cz; --ez) {
        if (map.is_explored(mx, my, ez))
          explored = true;
        if (map.is_visible(mx, my, ez))
          visible = true;
        if (explored && visible)
          break;
      }

      if (!explored) {
        for (int ez = z + 1; ez < map.get_depth(); ++ez) {
          if (map.is_explored(mx, my, ez)) {
            cz = ez;
            explored = true;
            visible = map.is_visible(mx, my, ez);
            break;
          }
        }
      }

      if (!explored) {
        plane.set(vx, vy, ' ', 0, 0);
        continue;
      }

      int surface_z = cz;
      char glyph = '0' + (surface_z % 10);
      int fg = 232 + std::min(23, surface_z / 4);
      int bg = contour_bg[(surface_z / 5) % 5];

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

      plane.set(vx, vy, glyph, fg, bg);
    }
  }
}
