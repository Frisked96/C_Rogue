#pragma once

#include "Entity/entity_manager.hpp"
#include "Object/object_manager.hpp"
#include "game_map.hpp"
#include <string>

class PhysicsSystem {
public:
  void update(EntityManager &entityManager, Game_map &map,
              ObjectManager &obj_mgr, std::string &last_msg);
};
