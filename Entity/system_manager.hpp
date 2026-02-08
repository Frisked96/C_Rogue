#pragma once
#include "entity_manager.hpp"
#include "anatomy_system.hpp"
#include "physiology_system.hpp"
#include "damage_resolution_system.hpp"
#include "spatial_system.hpp"
#include <unordered_set>

/**
 * @brief Aggregates all systems and provides a unified interface for the game world.
 * 
 * This class acts as the "orchestrator" for entity logic, ensuring systems run 
 * in the correct order and providing a single point of access for game logic.
 */
class SystemManager : public IEntityListener {
public:
    SystemManager();

    /**
     * @brief Updates all active systems for all relevant entities.
     * This handles anatomy, physiology, etc. in a single pass or coordinated passes.
     */
    void update(EntityManager& em);

    /**
     * @brief Resolves an attack between two entities.
     */
    AttackResult resolveAttack(Entity* attacker, Entity* defender, const DamageInfo& info);

    /**
     * @brief Inflicts a specific wound on an entity.
     */
    bool inflictWound(Entity* target, const std::string& partName, int damage, int bleedSeverity = 0);

    /**
     * @brief Access to the spatial grid for queries (raycasts, proximity, etc).
     */
    SpatialGrid& getSpatialGrid() { return spatialGrid; }
    const SpatialGrid& getSpatialGrid() const { return spatialGrid; }

    // System Accessors
    AnatomySystem& getAnatomy() { return anatomy; }
    PhysiologySystem& getPhysiology() { return physiology; }
    DamageResolutionSystem& getDamageResolution() { return damageResolution; }

    // IEntityListener Implementation (moved from EntityManager)
    void onEntitySignatureChanged(Entity* entity, Signature newSignature) override;
    void onEntityMoved(Entity* entity, int oldX, int oldY, int newX, int newY) override;
    void onEntityDestroyed(Entity* entity) override;

    // Hybrid Update Management
    void markActive(int entityId);
    void markInactive(int entityId);

private:
    AnatomySystem anatomy;
    PhysiologySystem physiology;
    DamageResolutionSystem damageResolution;
    SpatialGrid spatialGrid;

    std::unordered_set<int> active_anatomy_entities; // Entities needing updates this turn
};
