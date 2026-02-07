#pragma once

#include <string>

struct Tile {
  // Symbol of tile
  char glyph;

  // Properties
  bool is_walkable;
  bool is_transparent;
  bool is_explored;
  std::string name;

  // Terminal colors
  int fg_color;
  int bg_color;

  // Constructor
  Tile() = default;
  // sym for glyph
  Tile(char sym, std::string n, bool w, bool t)
      : glyph(sym), is_walkable(w), is_transparent(t), is_explored(false),
        name(n), fg_color(7), bg_color(0) {}

  // utility
  bool is_wall() const { return !is_walkable && !is_transparent; }
  bool is_floor() const { return is_walkable && is_transparent; }
};

namespace Tiles {
extern const Tile Wall;
extern const Tile Floor;
} // namespace Tiles
