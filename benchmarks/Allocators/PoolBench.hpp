#pragma once

#include <BenchmarkHarness.hpp>
#include <volt/memory/PoolAllocator.hpp>
#include <vector>

namespace volt::bench {

// 48-byte game engine Particle / Rigidbody struct
struct alignas(16) PhysicsParticle {
    float pos[3];
    float vel[3];
    float force[3];
    uint32_t id;
    uint32_t active;
    uint32_t layer;
};

inline void run_pool_allocator_benchmarks() {
    constexpr std::size_t POOL_CAPACITY = 10'000;
    // Workload: 10k initial allocs + 5k churn re-allocs = 15,000 total lifecycle ops
    constexpr std::size_t TOTAL_OPS = 15'000;

    // ------------------------------------------------------------------------
    // 1. Baseline: operator new and delete with fragmented churn
    // ------------------------------------------------------------------------
    BenchmarkSuite::instance().measure(
        "Pool Allocator",
        "new / delete with 50% churn (baseline)",
        "10k heap allocs, free 50% alternating, realloc 5k into holes, verify checksum",
        TOTAL_OPS,
        sizeof(PhysicsParticle),
        [&]() -> std::uint64_t {
            std::vector<PhysicsParticle*> active(POOL_CAPACITY, nullptr);
            std::uint64_t checksum = 0;

            // 1. Initial 10k allocations
            for (std::size_t i = 0; i < POOL_CAPACITY; ++i) {
                auto* p = new PhysicsParticle();
                p->id = static_cast<uint32_t>(i);
                p->pos[0] = static_cast<float>(i * 2);
                active[i] = p;
            }

            // 2. Churn: free every even slot (simulate entity deaths)
            for (std::size_t i = 0; i < POOL_CAPACITY; i += 2) {
                delete active[i];
                active[i] = nullptr;
            }

            // 3. Re-allocate 5,000 new entities into freed holes
            for (std::size_t i = 0; i < POOL_CAPACITY; i += 2) {
                auto* p = new PhysicsParticle();
                p->id = static_cast<uint32_t>(i + 100'000);
                p->pos[0] = static_cast<float>(i * 3);
                active[i] = p;
            }

            // 4. Verify checksum across all 10,000 active objects
            for (std::size_t i = 0; i < POOL_CAPACITY; ++i) {
                checksum += active[i]->id;
                delete active[i];
            }

            return checksum;
        }
    );

    // ------------------------------------------------------------------------
    // 2. Volt PoolAllocator: Same fragmented churn sequence
    // ------------------------------------------------------------------------
    BenchmarkSuite::instance().measure(
        "Pool Allocator",
        "volt::PoolAllocator with 50% churn",
        "10k pool allocs, free 50% alternating, realloc 5k into holes, verify checksum",
        TOTAL_OPS,
        sizeof(PhysicsParticle),
        [&]() -> std::uint64_t {
            volt::PoolAllocator<PhysicsParticle, POOL_CAPACITY> pool;
            std::vector<PhysicsParticle*> active(POOL_CAPACITY, nullptr);
            std::uint64_t checksum = 0;

            // 1. Initial 10k allocations
            for (std::size_t i = 0; i < POOL_CAPACITY; ++i) {
                auto* p = pool.allocate();
                p->id = static_cast<uint32_t>(i);
                p->pos[0] = static_cast<float>(i * 2);
                active[i] = p;
            }

            // 2. Churn: free every even slot (slot reuse in free list)
            for (std::size_t i = 0; i < POOL_CAPACITY; i += 2) {
                pool.deallocate(active[i]);
                active[i] = nullptr;
            }

            // 3. Re-allocate 5,000 new entities from pool free list
            for (std::size_t i = 0; i < POOL_CAPACITY; i += 2) {
                auto* p = pool.allocate();
                p->id = static_cast<uint32_t>(i + 100'000);
                p->pos[0] = static_cast<float>(i * 3);
                active[i] = p;
            }

            // 4. Verify checksum across all 10,000 active objects
            for (std::size_t i = 0; i < POOL_CAPACITY; ++i) {
                checksum += active[i]->id;
                pool.deallocate(active[i]);
            }

            return checksum;
        }
    );
}

} // namespace volt::bench
