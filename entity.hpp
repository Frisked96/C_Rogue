#pragma once
#include <string>
#include "point.hpp"
class Entity {
private:
  Point pos;
  int id;

  char glyph;

  bool blocks_movement;
  std::string name;

public:
  Entity(int x, int y, char c, const std::string& n, bool blocks = true);

  // getters
  char get_glyph() const { return glyph; }
  Point get_pos() const { return pos; }
  bool blocks() const { return blocks_movement; }
  const std::string &get_name() const { return name; }
  int get_id() const { return id; }

  // setters
  void set_position(int x, int y) {
    pos.x = x;
    pos.y = y;
  }
  void set_position(const Point& p) { pos = p; }
  void set_glyph(char c) { glyph = c; }

  // movement
  void move(int dx, int dy) {
    pos.x += dx;
    pos.y += dy;
  }
};
