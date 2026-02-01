#include "player.hpp"

Player::Player(int x, int y, const std::string& name, char glyph)
    : Entity(x, y, glyph, name, true) {
}
