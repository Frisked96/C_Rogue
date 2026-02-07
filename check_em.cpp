#include <iostream>
#include <cassert>
#include "entity_manager.hpp"
#include "components.hpp"

// Simple test for EntityManager
int main() {
    EntityManager em;

    std::cout << "Creating Entity 1..." << std::endl;
    Entity* e1 = em.createEntity();
    int id1 = e1->getId();
    std::cout << "Entity 1 ID: " << id1 << " Addr: " << e1 << std::endl;

    e1->addComponent<PositionComponent>(10, 20);
    e1->addComponent<NameComponent>("TestEntity");

    // Test Query
    std::cout << "Querying Position + Name..." << std::endl;
    auto results = em.getEntitiesWith<PositionComponent, NameComponent>();
    assert(results.size() == 1);
    assert(results[0] == e1);
    std::cout << "Query 1 Passed." << std::endl;

    std::cout << "Querying Position + Health (Should fail)..." << std::endl;
    auto results2 = em.getEntitiesWith<PositionComponent, HealthComponent>();
    assert(results2.size() == 0);
    std::cout << "Query 2 Passed." << std::endl;

    // Test Destruction and Pooling
    std::cout << "Destroying Entity 1..." << std::endl;
    em.destroyEntity(id1);

    auto results3 = em.getEntitiesWith<PositionComponent>();
    assert(results3.size() == 0);
    std::cout << "Entity removed from query." << std::endl;

    std::cout << "Creating Entity 2 (Should reuse memory)..." << std::endl;
    Entity* e2 = em.createEntity();
    std::cout << "Entity 2 ID: " << e2->getId() << " Addr: " << e2 << std::endl;

    // Check reuse
    if (e1 == e2) {
        std::cout << "Memory reused successfully!" << std::endl;
    } else {
        std::cout << "Memory NOT reused (Check pooling logic)." << std::endl;
    }
    
    // Check ID is different (reset called)
    if (e2->getId() != id1) {
        std::cout << "ID is new (correct)." << std::endl;
    } else {
        std::cout << "ID is same (incorrect reset)." << std::endl;
    }

    // Check clean state
    if (e2->hasComponent<PositionComponent>()) {
         std::cout << "State NOT cleared!" << std::endl;
    } else {
         std::cout << "State cleared successfully." << std::endl;
    }

    return 0;
}
