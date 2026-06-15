#pragma once

#include "tile.hpp"
#include <iostream>
#include <vector>

/*Takes in int x and int y as size, vector of Tiles
 */
class Game_map {
private:
  int width;
  int height;
  int depth;
  std::vector<Tile> map;

  int get_index(int x, int y, int z) const {
    return (z * width * height) + (y * width) + x;
  }

public:
  Game_map(int w, int h, int d);

  const Tile &get_tile(int x, int y, int z) const;
  void set_tile(int x, int y, int z, const Tile &tile);

  // check if position is walkable and within bounds
  bool can_walk(int x, int y, int z) const;

  int get_height() const { return height; }
  int get_width() const { return width; }
  int get_depth() const { return depth; }

  bool is_in_bounds(int x, int y, int z) const;

  void generate(int seed = 1337);

  // Visibility
  void clear_visibility();
  bool is_opaque(int x, int y, int z) const;
  void set_visible(int x, int y, int z, bool visible);
  bool is_visible(int x, int y, int z) const;
  bool is_explored(int x, int y, int z) const;
};
