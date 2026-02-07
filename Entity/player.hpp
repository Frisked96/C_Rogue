#pragma once
#include "entity.hpp"
#include <string>

class Player : public Entity {
public:
  // TODO: add proper comments for this
  Player(int x, int y, const std::string &name = "Player", char glyph = '@');

  // player specific methods
  void takeDamage(int amount);
  void heal(int amount);
};