#include "game_map.hpp"

Game_map::Game_map(int w, int h) : width(w), height(h) {
  map.resize(height);
  for (int y = 0; y < height; y++) {
    map[y].resize(width);
    for (int x = 0; x < width; x++) {
      map[y][x] = Tiles::Wall;
    }
  }
}

Tile Game_map::get_tile(int x, int y) const {
  if (x >= 0 && x < width && y >= 0 && y < height) {
    return map[y][x];
  }
  return Tiles::Wall;
}

void Game_map::set_tile(int x, int y, const Tile &tile) {
  if (x >= 0 && x < width && y >= 0 && y < height) {
    map[y][x] = tile;
  }
}

bool Game_map::can_walk(int x, int y) const {
  Tile tile = get_tile(x, y);
  return tile.is_walkable;
}

void Game_map::generate() {
  for (int y = 1; y < height - 1; y++) {
    for (int x = 1; x < width - 1; x++) {
      map[y][x] = Tiles::Floor;
    }
  }
}
