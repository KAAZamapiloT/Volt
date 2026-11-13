#if __has_include(<gtest/gtest.h>)
#include <gtest/gtest.h>
#else
#include "gtest_shim.hpp"
#endif

#include <volt/ECS/Registry.hpp>
#include <string>
#include <vector>

namespace {

struct Position {
    float x = 0.0f;
    float y = 0.0f;
    constexpr bool operator==(const Position&) const noexcept = default;
};

struct Velocity {
    float dx = 0.0f;
    float dy = 0.0f;
    constexpr bool operator==(const Velocity&) const noexcept = default;
};

struct Health {
    int hp = 100;
    int max_hp = 100;
    constexpr bool operator==(const Health&) const noexcept = default;
};

struct Tag {
    std::string name;
    constexpr bool operator==(const Tag&) const noexcept = default;
};

} // namespace

// ============================================================================
// 1. Entity Lifecycle Tests
// ============================================================================

TEST(VoltECSTest, EntityCreationAndLiveness) {
    volt::Registry reg;

    EXPECT_EQ(reg.entity_count(), 0u);

    volt::Entity e0 = reg.CreateEntity();
    volt::Entity e1 = reg.CreateEntity();
    volt::Entity e2 = reg.CreateEntity();

    EXPECT_NE(e0, e1);
    EXPECT_NE(e1, e2);
    EXPECT_NE(e0, e2);

    EXPECT_TRUE(reg.IsAlive(e0));
    EXPECT_TRUE(reg.IsAlive(e1));
    EXPECT_TRUE(reg.IsAlive(e2));

    EXPECT_EQ(reg.entity_count(), 3u);

    reg.DestroyEntity(e1);

    EXPECT_TRUE(reg.IsAlive(e0));
    EXPECT_FALSE(reg.IsAlive(e1));
    EXPECT_TRUE(reg.IsAlive(e2));

    EXPECT_EQ(reg.entity_count(), 2u);
}

// ============================================================================
// 2. Generational Handle Safety Test
// ============================================================================

TEST(VoltECSTest, GenerationalIndexSafety) {
    volt::Registry reg;

    volt::Entity old_handle = reg.CreateEntity();
    EXPECT_EQ(old_handle.id, 0u);
    EXPECT_EQ(old_handle.generation, 0u);
    EXPECT_TRUE(reg.IsAlive(old_handle));

    reg.DestroyEntity(old_handle);
    EXPECT_FALSE(reg.IsAlive(old_handle));

    // Creating a new entity should reuse ID 0 with incremented generation
    volt::Entity new_handle = reg.CreateEntity();
    EXPECT_EQ(new_handle.id, 0u);
    EXPECT_EQ(new_handle.generation, 1u);

    // Stale handle with generation 0 must be safely rejected
    EXPECT_FALSE(reg.IsAlive(old_handle));
    EXPECT_TRUE(reg.IsAlive(new_handle));
}

// ============================================================================
// 3. Component Emplace & Get Tests
// ============================================================================

TEST(VoltECSTest, ComponentEmplaceAndGet) {
    volt::Registry reg;
    volt::Entity e = reg.CreateEntity();

    EXPECT_FALSE(reg.hasComponent<Position>(e));
    EXPECT_FALSE(reg.hasComponent<Velocity>(e));

    reg.emplaceComponent<Position>(e, 10.5f, 20.25f);

    EXPECT_TRUE(reg.hasComponent<Position>(e));
    EXPECT_FALSE(reg.hasComponent<Velocity>(e));

    Position& pos = reg.getComponent<Position>(e);
    EXPECT_FLOAT_EQ(pos.x, 10.5f);
    EXPECT_FLOAT_EQ(pos.y, 20.25f);

    // Mutate through reference
    pos.x = 99.0f;
    EXPECT_FLOAT_EQ(reg.getComponent<Position>(e).x, 99.0f);

    // Const registry access
    const volt::Registry& const_reg = reg;
    const Position& const_pos = const_reg.getComponent<Position>(e);
    EXPECT_FLOAT_EQ(const_pos.x, 99.0f);

    // In-place update when component already exists
    reg.emplaceComponent<Position>(e, 100.0f, 200.0f);
    EXPECT_FLOAT_EQ(reg.getComponent<Position>(e).x, 100.0f);
    EXPECT_FLOAT_EQ(reg.getComponent<Position>(e).y, 200.0f);
}

// ============================================================================
// 4. Multi-Component Archetype Migration Test
// ============================================================================

TEST(VoltECSTest, MultiComponentMigration) {
    volt::Registry reg;
    volt::Entity e = reg.CreateEntity();

    // 1. Start with Position (Archetype: {Position})
    reg.emplaceComponent<Position>(e, 1.0f, 2.0f);
    EXPECT_TRUE(reg.hasComponent<Position>(e));

    // 2. Add Velocity (Archetype: {Position, Velocity})
    reg.emplaceComponent<Velocity>(e, 0.5f, -0.5f);
    EXPECT_TRUE(reg.hasComponent<Position>(e));
    EXPECT_TRUE(reg.hasComponent<Velocity>(e));
    // Verify Position was preserved during migration
    EXPECT_FLOAT_EQ(reg.getComponent<Position>(e).x, 1.0f);
    EXPECT_FLOAT_EQ(reg.getComponent<Position>(e).y, 2.0f);

    // 3. Add Health (Archetype: {Position, Velocity, Health})
    reg.emplaceComponent<Health>(e, 85, 100);
    EXPECT_TRUE(reg.hasComponent<Position>(e));
    EXPECT_TRUE(reg.hasComponent<Velocity>(e));
    EXPECT_TRUE(reg.hasComponent<Health>(e));

    // Verify all 3 components are intact
    EXPECT_FLOAT_EQ(reg.getComponent<Position>(e).x, 1.0f);
    EXPECT_FLOAT_EQ(reg.getComponent<Velocity>(e).dx, 0.5f);
    EXPECT_EQ(reg.getComponent<Health>(e).hp, 85);
}

// ============================================================================
// 5. Component Removal Test
// ============================================================================

TEST(VoltECSTest, ComponentRemoval) {
    volt::Registry reg;
    volt::Entity e = reg.CreateEntity();

    reg.emplaceComponent<Position>(e, 5.0f, 15.0f);
    reg.emplaceComponent<Velocity>(e, 1.0f, 2.0f);
    reg.emplaceComponent<Health>(e, 50, 100);

    // Remove Velocity
    reg.removeComponent<Velocity>(e);

    EXPECT_FALSE(reg.hasComponent<Velocity>(e));
    EXPECT_TRUE(reg.hasComponent<Position>(e));
    EXPECT_TRUE(reg.hasComponent<Health>(e));

    // Remaining components retain their data
    EXPECT_FLOAT_EQ(reg.getComponent<Position>(e).x, 5.0f);
    EXPECT_EQ(reg.getComponent<Health>(e).hp, 50);

    // Remove all components
    reg.removeAllComponents(e);

    EXPECT_FALSE(reg.hasComponent<Position>(e));
    EXPECT_FALSE(reg.hasComponent<Velocity>(e));
    EXPECT_FALSE(reg.hasComponent<Health>(e));
    EXPECT_TRUE(reg.IsAlive(e));
}

// ============================================================================
// 6. Swap-and-Pop Column Integrity Test
// ============================================================================

TEST(VoltECSTest, SwapAndPopIntegrity) {
    volt::Registry reg;

    volt::Entity e0 = reg.CreateEntity();
    volt::Entity e1 = reg.CreateEntity();
    volt::Entity e2 = reg.CreateEntity();

    reg.emplaceComponent<Position>(e0, 10.0f, 0.0f);
    reg.emplaceComponent<Position>(e1, 20.0f, 0.0f);
    reg.emplaceComponent<Position>(e2, 30.0f, 0.0f);

    // Destroy entity e0 (row 0). The last entity e2 (row 2) must be swapped into row 0!
    reg.DestroyEntity(e0);

    EXPECT_FALSE(reg.IsAlive(e0));
    EXPECT_TRUE(reg.IsAlive(e1));
    EXPECT_TRUE(reg.IsAlive(e2));

    // e2's Position must still be accessible and correct
    EXPECT_FLOAT_EQ(reg.getComponent<Position>(e2).x, 30.0f);
    EXPECT_FLOAT_EQ(reg.getComponent<Position>(e1).x, 20.0f);

    // Destroy e2 now
    reg.DestroyEntity(e2);
    EXPECT_FALSE(reg.IsAlive(e2));
    EXPECT_TRUE(reg.IsAlive(e1));
    EXPECT_FLOAT_EQ(reg.getComponent<Position>(e1).x, 20.0f);
}

// ============================================================================
// 7. System Query / Each Iteration Tests
// ============================================================================

TEST(VoltECSTest, QueryEach) {
    volt::Registry reg;

    // Create 30 entities with {Position, Velocity}
    for (int i = 0; i < 30; ++i) {
        auto e = reg.CreateEntity();
        reg.emplaceComponent<Position>(e, static_cast<float>(i), 0.0f);
        reg.emplaceComponent<Velocity>(e, 1.0f, 2.0f);
    }

    // Create 20 entities with {Position, Velocity, Health}
    for (int i = 0; i < 20; ++i) {
        auto e = reg.CreateEntity();
        reg.emplaceComponent<Position>(e, 100.0f, 0.0f);
        reg.emplaceComponent<Velocity>(e, 5.0f, 5.0f);
        reg.emplaceComponent<Health>(e, 100, 100);
    }

    // Create 15 entities with ONLY {Position}
    for (int i = 0; i < 15; ++i) {
        auto e = reg.CreateEntity();
        reg.emplaceComponent<Position>(e, -1.0f, -1.0f);
    }

    // Query 1: Each<Position, Velocity> with Entity handle parameter
    // Must match 30 + 20 = 50 entities
    std::size_t matched_count = 0;
    reg.Each<Position, Velocity>([&](volt::Entity e, Position& p, Velocity& v) {
        EXPECT_TRUE(reg.IsAlive(e));
        p.x += v.dx;
        p.y += v.dy;
        ++matched_count;
    });
    EXPECT_EQ(matched_count, 50u);

    // Query 2: Each<Position, Velocity> without Entity handle parameter
    matched_count = 0;
    reg.Each<Position, Velocity>([&](Position& p, Velocity& v) {
        (void)p;
        (void)v;
        ++matched_count;
    });
    EXPECT_EQ(matched_count, 50u);

    // Query 3: Each<Health> must match only 20 entities
    std::size_t health_count = 0;
    reg.Each<Health>([&](Health& h) {
        EXPECT_EQ(h.hp, 100);
        ++health_count;
    });
    EXPECT_EQ(health_count, 20u);
}

// ============================================================================
// 8. 10,000 Entities Stress Test
// ============================================================================

TEST(VoltECSTest, Stress10kEntities) {
    volt::Registry reg;
    constexpr std::size_t TOTAL = 10'000;
    std::vector<volt::Entity> entities;
    entities.reserve(TOTAL);

    for (std::size_t i = 0; i < TOTAL; ++i) {
        auto e = reg.CreateEntity();
        reg.emplaceComponent<Position>(e, static_cast<float>(i), static_cast<float>(i * 2));
        reg.emplaceComponent<Velocity>(e, 1.0f, 1.0f);
        entities.push_back(e);
    }

    EXPECT_EQ(reg.entity_count(), TOTAL);

    // Run movement system
    std::size_t processed = 0;
    reg.Each<Position, Velocity>([&](Position& p, Velocity& v) {
        p.x += v.dx;
        p.y += v.dy;
        ++processed;
    });
    EXPECT_EQ(processed, TOTAL);

    // Destroy 50% of entities (all even indices)
    for (std::size_t i = 0; i < TOTAL; i += 2) {
        reg.DestroyEntity(entities[i]);
    }

    EXPECT_EQ(reg.entity_count(), TOTAL / 2);

    // Verify all odd indices are still alive with correct values
    for (std::size_t i = 1; i < TOTAL; i += 2) {
        EXPECT_TRUE(reg.IsAlive(entities[i]));
        const auto& pos = reg.getComponent<Position>(entities[i]);
        EXPECT_FLOAT_EQ(pos.x, static_cast<float>(i + 1));
    }

    // Run system again over remaining 5,000
    processed = 0;
    reg.Each<Position, Velocity>([&](Position& p, Velocity& v) {
        (void)p;
        (void)v;
        ++processed;
    });
    EXPECT_EQ(processed, TOTAL / 2);
}

// ============================================================================
// Main Runner
// ============================================================================

int main(int argc, char** argv) {
#if __has_include(<gtest/gtest.h>)
    ::testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
#else
    (void)argc;
    (void)argv;
    return testing::UnitTest::GetInstance().RunAllTests();
#endif
}
