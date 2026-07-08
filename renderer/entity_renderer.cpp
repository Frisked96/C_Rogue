#include "entity_renderer.hpp"

void EntityRenderer::render(RenderPlane& plane, EntityManager& entityManager,
                            ObjectManager* objManager, const Game_map& map,
                            int z, int cam_x, int cam_y) {
  int width  = plane.width();
  int height = plane.height();
  int start_x = cam_x - width / 2;
  int start_y = cam_y - height / 2;

  // Render objects first so entities draw on top
  if (objManager) {
    auto objects = objManager->get_all_active();
    for (auto *obj : objects) {
      if (obj->z == z) {
        if (map.is_visible(obj->x, obj->y, obj->z)) {
          int vx = obj->x - start_x;
          int vy = obj->y - start_y;
          if (vx >= 0 && vx < width && vy >= 0 && vy < height) {
            const auto& proto = objManager->proto_db().get(obj->prototype_id);
            plane.set(vx, vy, proto.glyph, proto.fg_color, 0);
          }
        }
      }
    }
  }

  auto entities = entityManager.get_all_active();
  for (auto *entity : entities) {
    if (entity->state.z == z) {
      if (map.is_visible(entity->state.x, entity->state.y, entity->state.z) ||
          entity->type == EntityType::PLAYER) {
        int vx = entity->state.x - start_x;
        int vy = entity->state.y - start_y;
        if (vx >= 0 && vx < width && vy >= 0 && vy < height) {
          plane.set(vx, vy, entity->props().glyph, entity->props().fg_color | 8, 0);
        }
      }
    }
  }
}
