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
  std::vector<std::vector<Tile>> map;

public:
  Game_map(int w, int h);

  Tile get_tile(int x, int y) const;
  void set_tile(int x, int y, const Tile &tile);

  // check if position is walkable and within bounds
  bool can_walk(int x, int y) const;

  int get_height() const { return height; }
  int get_width() const { return width; }

  void generate();
};
