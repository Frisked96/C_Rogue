#include "game_map.hpp"

Game_map::Game_map(int w, int h, int d) : width(w), height(h), depth(d) {
  map.assign(width * height * depth, Tiles::Wall);
}

const Tile& Game_map::get_tile(int x, int y, int z) const {
  if (is_in_bounds(x, y, z)) {
    return map[get_index(x, y, z)];
  }
  return Tiles::Wall;
}

void Game_map::set_tile(int x, int y, int z, const Tile &tile) {
  if (is_in_bounds(x, y, z)) {
    map[get_index(x, y, z)] = tile;
  }
}

bool Game_map::can_walk(int x, int y, int z) const {
  if (!is_in_bounds(x, y, z)) return false;
  const auto& tile = get_tile(x, y, z);
  return tile.get_is_walkable();
}

bool Game_map::is_in_bounds(int x, int y, int z) const {
  return x >= 0 && x < width && y >= 0 && y < height && z >= 0 && z < depth;
}

void Game_map::generate() {
  // Fill everything with air first
  map.assign(width * height * depth, Tile(MaterialType::AIR));

  // Create a floor at z = 0
  int z = 0;
  for (int y = 0; y < height; y++) {
    for (int x = 0; x < width; x++) {
      if (x == 0 || x == width - 1 || y == 0 || y == height - 1) {
        set_tile(x, y, z, Tiles::Wall);
      } else {
        set_tile(x, y, z, Tiles::Floor);
      }
    }
  }
}
