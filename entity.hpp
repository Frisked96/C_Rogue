#pragma once
#include <string>
class Entity {
private:
  int pos_x;
  int pos_y;
  int id;

  char glyph;

  bool blocks_movement;
  std::string name;

public:
  Entity(int x, int y, char c, const std::string& n, bool blocks = true);

  // getters
  char get_glyph() const { return glyph; }
  int get_pos_x() const { return pos_x; }
  int get_pos_y() const { return pos_y; }
  bool blocks() const { return blocks_movement; }
  const std::string &get_name() const { return name; }
  int get_id() const { return id; }

  // setters
  void set_position(int x, int y) {
    pos_x = x;
    pos_y = y;
  }
  void get_glyph(char c) { glyph = c; }

  // movement
  void move(int dx, int dy) {
    pos_x += dx;
    pos_y += dy;
  }
};
