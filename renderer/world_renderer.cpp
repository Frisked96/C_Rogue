#include "world_renderer.hpp"
#include <algorithm>

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

      if (!map.is_explored(mx, my, z)) {
        plane.set(vx, vy, ' ', 0, 0);
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

      if (!map.is_explored(mx, my, z)) {
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

      plane.set(vx, vy, glyph, fg, bg);
    }
  }
}
