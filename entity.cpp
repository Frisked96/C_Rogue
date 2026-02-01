#include "entity.hpp"

// static counter for unique IDs
static int next_id = 0;

Entity::Entity(int x, int y, char c, const std::string& n, bool blocks)
    : pos_x(x), pos_y(y), glyph(c), blocks_movement(blocks), name(n) {
  id = next_id++;
}
