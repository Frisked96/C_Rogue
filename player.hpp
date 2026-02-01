#pragma once
#include "entity.hpp"

class Player : public Entity {
public:
    Player(int x, int y, const std::string& name = "Player", char glyph = '@');
};
