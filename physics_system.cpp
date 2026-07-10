#include "physics_system.hpp"

void PhysicsSystem::update(EntityManager &entityManager, Game_map &map,
                           ObjectManager &obj_mgr, std::string &last_msg) {
  for (Entity *entity : entityManager.get_all_active()) {
    if (!entity->state.has_intent_to_move)
      continue;

    int dx = entity->state.intent_dx;
    int dy = entity->state.intent_dy;
    int dz = entity->state.intent_dz;

    if (dx != 0 || dy != 0 || dz != 0) {
      int nx = entity->state.x + dx;
      int ny = entity->state.y + dy;
      int nz = entity->state.z + dz;

      // Surface-following logic
      bool is_blocked_base = entityManager.is_blocked(nx, ny, nz, map);
      if (!is_blocked_base && obj_mgr.spatial().has_any(nx, ny, nz)) {
        for (auto uid : obj_mgr.spatial().get_at(nx, ny, nz)) {
          auto *obj = obj_mgr.get(uid);
          if (obj && obj_mgr.proto_db().get(obj->prototype_id).is_blocking) {
            is_blocked_base = true;
            break;
          }
        }
      }

      if (is_blocked_base) {
        // Attempt to climb (up to 2m)
        bool is_blocked_z1 = entityManager.is_blocked(nx, ny, nz + 1, map);
        if (!is_blocked_z1 && obj_mgr.spatial().has_any(nx, ny, nz + 1)) {
          for (auto uid : obj_mgr.spatial().get_at(nx, ny, nz + 1)) {
            auto *obj = obj_mgr.get(uid);
            if (obj && obj_mgr.proto_db().get(obj->prototype_id).is_blocking) {
              is_blocked_z1 = true;
              break;
            }
          }
        }
        if (!is_blocked_z1) {
          nz++;
          if (entity->type == EntityType::PLAYER) last_msg = "You climb up.";
        } else {
          bool is_blocked_z2 = entityManager.is_blocked(nx, ny, nz + 2, map);
          if (!is_blocked_z2 && obj_mgr.spatial().has_any(nx, ny, nz + 2)) {
            for (auto uid : obj_mgr.spatial().get_at(nx, ny, nz + 2)) {
              auto *obj = obj_mgr.get(uid);
              if (obj && obj_mgr.proto_db().get(obj->prototype_id).is_blocking) {
                is_blocked_z2 = true;
                break;
              }
            }
          }
          if (!is_blocked_z2) {
            nz += 2;
            if (entity->type == EntityType::PLAYER) last_msg = "You scramble up the ridge.";
          } else {
            if (entity->type == EntityType::PLAYER) last_msg = "Blocked by " + map.get_tile(nx, ny, nz).mat().name + " or object.";
            entity->state.has_intent_to_move = false;
            continue;
          }
        }
      } else {
        // Gravity / Descending logic
        int start_z = nz;
        auto is_blocked_at = [&](int cx, int cy, int cz) {
          if (entityManager.is_blocked(cx, cy, cz, map))
            return true;
          if (obj_mgr.spatial().has_any(cx, cy, cz)) {
            for (auto uid : obj_mgr.spatial().get_at(cx, cy, cz)) {
              auto *obj = obj_mgr.get(uid);
              if (obj && obj_mgr.proto_db().get(obj->prototype_id).is_blocking)
                return true;
            }
          }
          return false;
        };

        while (nz > 0 && !is_blocked_at(nx, ny, nz) && !is_blocked_at(nx, ny, nz - 1)) {
          // Buoyancy: Stop falling if we hit deep enough water
          const Tile &current_tile = map.get_tile(nx, ny, nz);
          if (current_tile.material == MaterialType::AIR &&
              current_tile.state.liquid_volume >= 0.4f) {
            break;
          }
          nz--;
        }

        if (nz < start_z) {
          if (entity->type == EntityType::PLAYER) last_msg = (start_z - nz > 1) ? "You scramble down." : "You descend.";
        } else if (map.get_tile(nx, ny, nz).material == MaterialType::AIR &&
                   map.get_tile(nx, ny, nz).state.liquid_volume > 0.0f) {
          float m = map.get_tile(nx, ny, nz).state.liquid_volume;
          if (entity->type == EntityType::PLAYER) {
            if (m >= 0.8f)
              last_msg = "You are swimming.";
            else if (m >= 0.4f)
              last_msg = "You wade through waist-deep water.";
            else
              last_msg = "You splash through ankle-deep water.";
          }
        } else {
          if (entity->type == EntityType::PLAYER) last_msg = "You move forward.";
        }
      }

      auto is_blocked_at_final = [&](int cx, int cy, int cz) {
        if (entityManager.is_blocked(cx, cy, cz, map))
          return true;
        if (obj_mgr.spatial().has_any(cx, cy, cz)) {
          for (auto uid : obj_mgr.spatial().get_at(cx, cy, cz)) {
            auto *obj = obj_mgr.get(uid);
            if (obj && obj_mgr.proto_db().get(obj->prototype_id).is_blocking)
              return true;
          }
        }
        return false;
      };

      if (!is_blocked_at_final(nx, ny, nz)) {
        entityManager.get_spatial_grid().move(entity->id, entity->state.x,
                                              entity->state.y, entity->state.z,
                                              nx, ny, nz);
        entity->state.x = nx;
        entity->state.y = ny;
        entity->state.z = nz;
      }
    }

    // Clear intent
    entity->state.has_intent_to_move = false;
    entity->state.intent_dx = 0;
    entity->state.intent_dy = 0;
    entity->state.intent_dz = 0;
  }
}
