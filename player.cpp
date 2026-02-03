#include "player.hpp"
#include "components.hpp"

Player::Player(int x, int y, const std::string& name, char glyph) {
    // components for player 
    addComponent<PositionComponent>(x, y);
    addComponent<RenderComponent>(glyph);
    addComponent<NameComponent>(name);
    addComponent<BlockingComponent>();
}

 // when we add health component 
void Player::takeDamage(int amount) {
        // empty for now 
}

void Player::heal(int amount) {
    // empty for now 
}