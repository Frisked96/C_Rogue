// Standalone test driver for the Object System.
// Compile separately -- not linked into the game.
// Usage: clang++ -std=c++20 -I.. -o object_test.exe Object/object_system_test.cpp Object/event_bus.cpp Object/object_prototype_db.cpp Object/object_manager.cpp material.cpp

#include "event_bus.hpp"
#include "object_manager.hpp"
#include "object_prototype_db.hpp"
#include <cstdio>
#include <cstdlib>

static int tests_passed = 0;
static int tests_failed = 0;

#define TEST(name) printf("  TEST: %-40s ", name);
#define PASS() do { printf("[PASS]\n"); tests_passed++; } while(0)
#define FAIL(msg) do { printf("[FAIL] %s\n", msg); tests_failed++; } while(0)
#define ASSERT(cond, msg) do { if (!(cond)) { FAIL(msg); return; } } while(0)

static void test_prototype_db() {
    TEST("Prototype DB load and lookup");
    ObjectPrototypeDB db;
    db.load_defaults();
    ASSERT(db.size() > 1, "Should have prototypes loaded");
    const auto& oak = db.get(1); // First registered after NULL
    ASSERT(oak.name == "Oak Tree", "First prototype should be Oak Tree");
    ASSERT(oak.is_static == true, "Oak Tree should be static");
    ASSERT(oak.is_living == false, "Oak Tree should not be living");
    PASS();
}

static void test_event_bus_dirty_flags() {
    TEST("Event bus dirty flags");
    EventBus bus;
    ASSERT(!bus.is_dirty(EventType::OBJECT_SPAWNED), "Should start clean");
    bus.announce(EventType::OBJECT_SPAWNED, {1, 0, 0.0f});
    ASSERT(bus.is_dirty(EventType::OBJECT_SPAWNED), "Should be dirty after announce");
    ASSERT(!bus.is_dirty(EventType::OBJECT_MOVED), "Other events should be clean");
    bus.clear_dirty();
    ASSERT(!bus.is_dirty(EventType::OBJECT_SPAWNED), "Should be clean after clear");
    PASS();
}

static void test_event_bus_subscribe() {
    TEST("Event bus subscribe/callback");
    EventBus bus;
    int callback_count = 0;
    uint32_t received_uid = 0;
    bus.subscribe(EventType::OBJECT_MOVED, [&](const EventPayload& p) {
        callback_count++;
        received_uid = p.source_uid;
    });
    bus.announce(EventType::OBJECT_MOVED, {42, 0, 0.0f});
    ASSERT(callback_count == 1, "Callback should fire once");
    ASSERT(received_uid == 42, "Should receive correct UID");
    bus.announce(EventType::OBJECT_MOVED, {99, 0, 0.0f});
    ASSERT(callback_count == 2, "Callback should fire again");
    PASS();
}

static void test_event_bus_unsubscribe() {
    TEST("Event bus unsubscribe");
    EventBus bus;
    int count = 0;
    uint32_t sub_id = bus.subscribe(EventType::HEALTH_CHANGED, [&](const EventPayload&) {
        count++;
    });
    bus.announce(EventType::HEALTH_CHANGED);
    ASSERT(count == 1, "Should fire before unsub");
    bus.unsubscribe(EventType::HEALTH_CHANGED, sub_id);
    bus.announce(EventType::HEALTH_CHANGED);
    ASSERT(count == 1, "Should not fire after unsub");
    PASS();
}

static void test_spawn_and_get() {
    TEST("Spawn and get object");
    ObjectPrototypeDB db;
    db.load_defaults();
    EventBus bus;
    ObjectManager mgr(db, bus);

    // Spawn an Oak Tree (prototype 1)
    ObjectUID tree_uid = mgr.spawn(1, 5, 10, 0);
    ASSERT(tree_uid != INVALID_OBJECT_UID, "Should get valid UID");
    auto* tree = mgr.get(tree_uid);
    ASSERT(tree != nullptr, "Should be able to get tree");
    ASSERT(tree->x == 5 && tree->y == 10 && tree->z == 0, "Position should match");
    ASSERT(tree->life == nullptr, "Non-living should not have LifeState");
    PASS();
}

static void test_spawn_living() {
    TEST("Spawn living object gets LifeState");
    ObjectPrototypeDB db;
    db.load_defaults();
    EventBus bus;
    ObjectManager mgr(db, bus);

    // Human NPC is prototype 6
    ObjectUID npc_uid = mgr.spawn(6, 3, 3, 0);
    auto* npc = mgr.get(npc_uid);
    ASSERT(npc != nullptr, "Should get NPC");
    ASSERT(npc->life != nullptr, "Living object should have LifeState");
    ASSERT(npc->life->profession == ProfessionType::FARMER, "Default job should be FARMER");
    ASSERT(npc->life->current_goal == GoalType::IDLE, "Should start IDLE");
    PASS();
}

static void test_move_object() {
    TEST("Move object updates position and spatial grid");
    ObjectPrototypeDB db;
    db.load_defaults();
    EventBus bus;
    ObjectManager mgr(db, bus);

    ObjectUID uid = mgr.spawn(6, 0, 0, 0);
    mgr.move(uid, 5, 5, 1);
    auto* obj = mgr.get(uid);
    ASSERT(obj->x == 5 && obj->y == 5 && obj->z == 1, "Position should update");

    // Should be findable at new position
    const auto& at_new = mgr.spatial().get_at(5, 5, 1);
    ASSERT(!at_new.empty() && at_new[0] == uid, "Should be at new position");

    // Should NOT be at old position
    const auto& at_old = mgr.spatial().get_at(0, 0, 0);
    ASSERT(at_old.empty(), "Should not be at old position");
    PASS();
}

static void test_kill_and_reuse() {
    TEST("Kill object and UID reuse");
    ObjectPrototypeDB db;
    db.load_defaults();
    EventBus bus;
    ObjectManager mgr(db, bus);

    ObjectUID uid1 = mgr.spawn(1, 0, 0, 0);
    mgr.kill(uid1);
    ASSERT(mgr.get(uid1) == nullptr, "Killed object should be null");

    // Spawn another -- should reuse the slot but with new generation
    ObjectUID uid2 = mgr.spawn(1, 1, 1, 0);
    ASSERT(uid2 != uid1, "New UID should differ (different generation)");
    ASSERT(mgr.get(uid2) != nullptr, "New object should be valid");
    PASS();
}

static void test_tick_medium_hunger() {
    TEST("Medium tick increases hunger");
    ObjectPrototypeDB db;
    db.load_defaults();
    EventBus bus;
    ObjectManager mgr(db, bus);

    ObjectUID uid = mgr.spawn(6, 0, 0, 0);  // Human NPC
    auto* npc = mgr.get(uid);
    float initial_hunger = npc->life->hunger;

    // Run 10 turns to trigger a medium tick
    for (int i = 0; i < 10; i++) {
        mgr.tick();
    }

    npc = mgr.get(uid);
    ASSERT(npc->life->hunger > initial_hunger, "Hunger should increase after medium tick");
    PASS();
}

static void test_dirty_flag_integration() {
    TEST("Dirty flags set during spawn/move/kill");
    ObjectPrototypeDB db;
    db.load_defaults();
    EventBus bus;
    ObjectManager mgr(db, bus);

    // Spawn sets OBJECT_SPAWNED dirty
    ObjectUID uid = mgr.spawn(1, 0, 0, 0);
    ASSERT(bus.is_dirty(EventType::OBJECT_SPAWNED), "Spawn should set dirty");
    bus.clear_dirty();

    // Move sets OBJECT_MOVED dirty
    mgr.move(uid, 1, 1, 0);
    ASSERT(bus.is_dirty(EventType::OBJECT_MOVED), "Move should set dirty");
    bus.clear_dirty();

    // Kill sets OBJECT_KILLED dirty
    mgr.kill(uid);
    ASSERT(bus.is_dirty(EventType::OBJECT_KILLED), "Kill should set dirty");
    PASS();
}

static void test_spatial_grid_query() {
    TEST("Spatial grid multi-object query");
    ObjectPrototypeDB db;
    db.load_defaults();
    EventBus bus;
    ObjectManager mgr(db, bus);

    ObjectUID a = mgr.spawn(1, 5, 5, 0);
    ObjectUID b = mgr.spawn(5, 5, 5, 0);  // Apple at same position
    const auto& at = mgr.spatial().get_at(5, 5, 0);
    ASSERT(at.size() == 2, "Should have 2 objects at same position");
    (void)a; (void)b;
    PASS();
}

int main() {
    printf("\n=== Object System Tests ===\n\n");

    test_prototype_db();
    test_event_bus_dirty_flags();
    test_event_bus_subscribe();
    test_event_bus_unsubscribe();
    test_spawn_and_get();
    test_spawn_living();
    test_move_object();
    test_kill_and_reuse();
    test_tick_medium_hunger();
    test_dirty_flag_integration();
    test_spatial_grid_query();

    printf("\n=== Results: %d passed, %d failed ===\n\n", tests_passed, tests_failed);
    return tests_failed > 0 ? EXIT_FAILURE : EXIT_SUCCESS;
}
