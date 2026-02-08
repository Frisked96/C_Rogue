#pragma once

#include "entity_manager.hpp"
#include <string>

class EntityFactory {
private:
  EntityManager &entityManager;

public:
  EntityFactory(EntityManager &em);

  Entity *createPlayer(int x, int y, const std::string &name = "Player",
                       char glyph = '@');

  // Future expansion:
  // Entity* createMonster(int x, int y, const std::string& name);
};
