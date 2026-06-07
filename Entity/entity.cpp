#include "entity.hpp"
#include "components.hpp"

int Entity::next_id = 0;

Entity::Entity() : id(next_id++) {}

void Entity::setPosition(int x, int y, int z) {
  int oldX = 0, oldY = 0, oldZ = 0;
  bool hadPos = false;
  if (auto *pos = getComponent<PositionComponent>()) {
    oldX = pos->x;
    oldY = pos->y;
    oldZ = pos->z;
    hadPos = true;
    pos->x = x;
    pos->y = y;
    pos->z = z;
  } else {
    addComponent<PositionComponent>(x, y, z);
  }

  if (listener) {
    listener->onEntityMoved(this, hadPos ? oldX : x, hadPos ? oldY : y, hadPos ? oldZ : z, x, y, z);
  }
}

void Entity::move(int dx, int dy, int dz) {
  if (auto *pos = getComponent<PositionComponent>()) {
    int oldX = pos->x;
    int oldY = pos->y;
    int oldZ = pos->z;
    pos->x += dx;
    pos->y += dy;
    pos->z += dz;
    if (listener) {
      listener->onEntityMoved(this, oldX, oldY, oldZ, pos->x, pos->y, pos->z);
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

bool Entity::hasEnvironment() const {
  return hasComponent<EnvironmentComponent>();
}

EnvironmentComponent *Entity::getEnvironment() {
  return getComponent<EnvironmentComponent>();
}

bool Entity::hasSpatialProfile() const {
  return hasComponent<SpatialProfileComponent>();
}

SpatialProfileComponent *Entity::getSpatialProfile() {
  return getComponent<SpatialProfileComponent>();
}