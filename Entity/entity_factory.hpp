#pragma once

#include "entity_manager.hpp"
#include "body_template.hpp"
#include <string>

class EntityFactory {
private:
  EntityManager &entityManager;

public:
  EntityFactory(EntityManager &em);

  Entity *createPlayer(int x, int y, const std::string &name = "Player",
                       char glyph = '@');

  Entity *createFromTemplate(int x, int y, const BodyTemplate &bodyTemplate,
                             const std::string &name, char glyph);

  static BodyTemplate createHumanTemplate();
};
