#include "entity.hpp"
#include "components.hpp"

int Entity::next_id = 0;

Entity::Entity() : id(next_id++) {}

void Entity::setPosition(int x, int y) {
  if (auto* pos = getComponent<PositionComponent>()) {
    pos->x = x;
    pos->y = y;
  } else {
    addComponent<PositionComponent>(x, y);
  }
}

void Entity::move(int dx, int dy) {
  if (auto* pos = getComponent<PositionComponent>()) {
    pos->x += dx;
    pos->y += dy;
  }
}

char Entity::getGlyph() const {
  if (auto* render = getComponent<RenderComponent>()) {
    return render->glyph;
  }
  return '?';
}

std::string Entity::getName() const {
  if (auto* name = getComponent<NameComponent>()) {
    return name->name;
  }
  return "";
}

bool Entity::blocksMovement() const {
  return hasComponent<BlockingComponent>();
}
