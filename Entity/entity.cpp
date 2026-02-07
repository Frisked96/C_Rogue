#include "entity.hpp"
#include "components.hpp"

int Entity::next_id = 0;

Entity::Entity() : id(next_id++) {}

void Entity::setPosition(int x, int y) {
  int oldX = 0, oldY = 0;
  bool hadPos = false;
  if (auto *pos = getComponent<PositionComponent>()) {
    oldX = pos->x;
    oldY = pos->y;
    hadPos = true;
    pos->x = x;
    pos->y = y;
  } else {
    addComponent<PositionComponent>(x, y);
  }

  if (listener) {
    listener->onEntityMoved(this, hadPos ? oldX : x, hadPos ? oldY : y, x, y);
  }
}

void Entity::move(int dx, int dy) {
  if (auto *pos = getComponent<PositionComponent>()) {
    int oldX = pos->x;
    int oldY = pos->y;
    pos->x += dx;
    pos->y += dy;
    if (listener) {
      listener->onEntityMoved(this, oldX, oldY, pos->x, pos->y);
    }
  }
}

char Entity::getGlyph() const {
  if (auto *render = getComponent<RenderComponent>()) {
    return render->glyph;
  }
  return '?';
}

std::string Entity::getName() const {
  if (auto *name = getComponent<NameComponent>()) {
    return name->name;
  }
  return "";
}

bool Entity::blocksMovement() const {
  return hasComponent<BlockingComponent>();
}

bool Entity::hasHealth() const { return hasComponent<HealthComponent>(); }

HealthComponent *Entity::getHealth() { return getComponent<HealthComponent>(); }

bool Entity::hasAnatomy() const { return hasComponent<AnatomyComponent>(); }

AnatomyComponent *Entity::getAnatomy() {
  return getComponent<AnatomyComponent>();
}

bool Entity::hasCombat() const { return hasComponent<CombatComponent>(); }

CombatComponent *Entity::getCombat() { return getComponent<CombatComponent>(); }

bool Entity::hasInventory() const { return hasComponent<InventoryComponent>(); }

InventoryComponent *Entity::getInventory() {
  return getComponent<InventoryComponent>();
}