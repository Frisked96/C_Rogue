#include "entity_renderer.hpp"
#include <algorithm>

void EntityRenderer::render(RenderPlane &plane, EntityManager &entityManager,
                            ObjectManager *objManager, const Game_map &map,
                            int z, int cam_x, int cam_y) {
  int width = plane.width();
  int height = plane.height();
  int start_x = cam_x - width / 2;
  int start_y = cam_y - height / 2;

  // Instead of iterating all objects/entities and filtering by z==playerZ,
  // iterate by screen cell and scan the column from z downward.
  // This lets us see objects (trees etc.) on lower explored ground.

  for (int vy = 0; vy < height; ++vy) {
    for (int vx = 0; vx < width; ++vx) {
      int mx = start_x + vx;
      int my = start_y + vy;

      if (!map.is_in_bounds(mx, my, z))
        continue;

      // Raycast: find the surface z for this column (same as world_renderer)
      int cz = z;
      const Tile *t = &map.get_tile(mx, my, cz);
      while (cz > 0 && t->material == MaterialType::AIR &&
             t->state.liquid_volume <= 0.0f && t->state.frozen_volume <= 0.0f) {
        cz--;
        t = &map.get_tile(mx, my, cz);
      }

      // Check explored/visible across the column
      bool explored = false;
      bool visible = false;
      for (int ez = z; ez >= cz; --ez) {
        if (map.is_explored(mx, my, ez))
          explored = true;
        if (map.is_visible(mx, my, ez))
          visible = true;
        if (explored && visible)
          break;
      }

      int search_top = map.get_depth() - 1;

      if (!explored) {
        // If our current z-level wasn't explored, check if we remember a higher
        // surface
        for (int ez = z + 1; ez < map.get_depth(); ++ez) {
          if (map.is_explored(mx, my, ez)) {
            cz = ez;
            explored = true;
            visible = map.is_visible(mx, my, ez);
            break;
          }
        }
      }

      if (!explored)
        continue;

      // --- Render the highest object in the visible column ---
      if (objManager) {
        for (int oz = search_top; oz >= cz; --oz) {
          if (objManager->spatial().has_any(mx, my, oz)) {
            const auto &uids = objManager->spatial().get_at(mx, my, oz);
            for (auto uid : uids) {
              auto *obj = objManager->get(uid);
              if (!obj)
                continue;
              const auto &proto = objManager->proto_db().get(obj->prototype_id);

              int fg = proto.fg_color;
              if (!visible) {
                // Explored but not visible: grey memory
                fg = 237;
              } else {
                // Depth-dim objects below the player
                int depth = z - oz;
                if (depth > 0) {
                  if (fg >= 8 && fg <= 15)
                    fg -= 8;
                  if (depth > 2)
                    fg = std::max(232, 255 - (depth * 2));
                }
              }
              plane.set(vx, vy, proto.glyph, fg, 0);
              goto next_cell; // object found, skip to next cell
            }
          }
        }
      }

      // --- Render the highest entity in the visible column ---
      for (int ez = search_top; ez >= cz; --ez) {
        if (entityManager.get_spatial_grid().has_any(mx, my, ez)) {
          const auto &uids =
              entityManager.get_spatial_grid().get_at(mx, my, ez);
          for (auto uid : uids) {
            auto *entity = entityManager.get(uid);
            if (!entity)
              continue;
            if (visible || entity->type == EntityType::PLAYER) {
              int fg = entity->props().fg_color | 8;
              int depth = z - ez;
              if (depth > 0 && entity->type != EntityType::PLAYER) {
                if (fg >= 8 && fg <= 15)
                  fg -= 8;
                if (depth > 2)
                  fg = std::max(232, 255 - (depth * 2));
              }
              plane.set(vx, vy, entity->props().glyph, fg, 0);
              goto next_cell;
            }
          }
        }
      }

    next_cell:;
    }
  }
}
