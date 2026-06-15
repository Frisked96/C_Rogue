#include "visibility.hpp"
#include <cmath>
#include <vector>

struct Row {
  int depth;
  float start_slope;
  float end_slope;
};

void compute_sector(Game_map &map, int start_x, int start_y, int start_z,
                    int radius, int octant);

void Visibility::compute_fov(Game_map &map, int start_x, int start_y,
                             int start_z, int radius) {
  map.clear_visibility();
  map.set_visible(start_x, start_y, start_z, true);

  for (int i = 0; i < 8; ++i) {
    compute_sector(map, start_x, start_y, start_z, radius, i);
  }
}

float get_slope(int x, int y) { return (float)x / (float)y; }

void compute_sector(Game_map &map, int start_x, int start_y, int start_z,
                    int radius, int octant) {
  std::vector<Row> stack;
  stack.push_back({1, -1.0f, 1.0f});

  while (!stack.empty()) {
    Row row = stack.back();
    stack.pop_back();

    if (row.depth > radius)
      continue;

    float prev_opaque = false;

    for (int col = (int)std::floor(row.start_slope * row.depth + 0.5f);
         col <= (int)std::floor(row.end_slope * row.depth + 0.5f); ++col) {
      int x, y;
      // Map octant to (x, y)
      switch (octant) {
      case 0:
        x = start_x + col;
        y = start_y - row.depth;
        break; // N
      case 1:
        x = start_x + row.depth;
        y = start_y - col;
        break; // E
      case 2:
        x = start_x + row.depth;
        y = start_y + col;
        break; // E
      case 3:
        x = start_x + col;
        y = start_y + row.depth;
        break; // S
      case 4:
        x = start_x - col;
        y = start_y + row.depth;
        break; // S
      case 5:
        x = start_x - row.depth;
        y = start_y + col;
        break; // W
      case 6:
        x = start_x - row.depth;
        y = start_y - col;
        break; // W
      case 7:
        x = start_x - col;
        y = start_y - row.depth;
        break; // N
      }

      if (!map.is_in_bounds(x, y, start_z))
        continue;

      // Distance check for circular FOV
      if (col * col + row.depth * row.depth > radius * radius)
        continue;

      bool opaque = map.is_opaque(x, y, start_z);

      // Mark the whole column visible if the top is visible (for renderer
      // consistency)
      int cz = start_z;
      map.set_visible(x, y, cz, true);
      while (cz > 0 && map.get_tile(x, y, cz).material == MaterialType::AIR) {
        cz--;
        map.set_visible(x, y, cz, true);
      }

      if (opaque) {
        if (!prev_opaque && col > row.start_slope * row.depth) {
          stack.push_back(
              {row.depth + 1, row.start_slope, get_slope(col - 1, row.depth)});
        }
      } else {
        if (prev_opaque) {
          row.start_slope = get_slope(col, row.depth);
        }
      }
      prev_opaque = opaque;
    }

    if (!prev_opaque) {
      stack.push_back({row.depth + 1, row.start_slope, row.end_slope});
    }
  }
}
