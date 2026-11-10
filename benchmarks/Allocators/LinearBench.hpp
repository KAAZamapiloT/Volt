#pragma once

#include <BenchmarkHarness.hpp>
#include <volt/memory/LinearAllocator.hpp>
#include <memory_resource>
#include <vector>

namespace volt::bench {

// Realistic engine data payload (64 bytes aligned to cache line)
struct alignas(64) Transform64 {
    float pos[4];
    float rot[4];
    float scale[4];
    uint32_t entity_id;
    uint32_t flags;
    uint64_t timestamp;
};

inline void run_linear_allocator_benchmarks() {
    constexpr std::size_t ALLOC_COUNT = 50'000;
    constexpr std::size_t BLOCK_SIZE = sizeof(Transform64); // 64 bytes
    constexpr std::size_t ARENA_SIZE = ALLOC_COUNT * BLOCK_SIZE + 4096; // ~3.2 MiB

    std::vector<std::byte> raw_buffer(ARENA_SIZE);

    // ------------------------------------------------------------------------
    // 1. Baseline: malloc + write payload + read checksum + individual free()
    // ------------------------------------------------------------------------
    BenchmarkSuite::instance().measure(
        "Linear Allocator",
        "malloc + write + free (baseline)",
        "Heap allocation; dirty writes each block; reads checksum; individual free()",
        ALLOC_COUNT,
        BLOCK_SIZE,
        [&]() -> std::uint64_t {
            std::vector<Transform64*> ptrs(ALLOC_COUNT);
            std::uint64_t checksum = 0;

            for (std::size_t i = 0; i < ALLOC_COUNT; ++i) {
                auto* t = static_cast<Transform64*>(std::malloc(sizeof(Transform64)));
                t->pos[0] = static_cast<float>(i);
                t->entity_id = static_cast<uint32_t>(i);
                ptrs[i] = t;
            }

            for (std::size_t i = 0; i < ALLOC_COUNT; ++i) {
                checksum += ptrs[i]->entity_id;
                std::free(ptrs[i]);
            }
            return checksum;
        }
    );

    // ------------------------------------------------------------------------
    // 2. Baseline: std::pmr::monotonic_buffer_resource (Arena allocate + write + release)
    // ------------------------------------------------------------------------
    BenchmarkSuite::instance().measure(
        "Linear Allocator",
        "std::pmr::monotonic (baseline)",
        "Preallocated arena; bump-pointer; dirty writes; reads checksum; arena release()",
        ALLOC_COUNT,
        BLOCK_SIZE,
        [&]() -> std::uint64_t {
            std::pmr::monotonic_buffer_resource pool(
                raw_buffer.data(), raw_buffer.size(), std::pmr::null_memory_resource());
            
            std::uint64_t checksum = 0;
            std::vector<Transform64*> ptrs(ALLOC_COUNT);

            for (std::size_t i = 0; i < ALLOC_COUNT; ++i) {
                auto* t = static_cast<Transform64*>(pool.allocate(sizeof(Transform64), alignof(Transform64)));
                t->pos[0] = static_cast<float>(i);
                t->entity_id = static_cast<uint32_t>(i);
                ptrs[i] = t;
            }

            for (std::size_t i = 0; i < ALLOC_COUNT; ++i) {
                checksum += ptrs[i]->entity_id;
            }

            pool.release();
            return checksum;
        }
    );

    // ------------------------------------------------------------------------
    // 3. Volt LinearAllocator: Full Lifecycle (Alloc + Write + Read + O(1) Reset)
    // ------------------------------------------------------------------------
    {
        volt::LinearAllocator alloc(raw_buffer.data(), raw_buffer.size());
        BenchmarkSuite::instance().measure(
            "Linear Allocator",
            "volt::LinearAllocator (alloc + write + reset)",
            "Volt bump allocator; dirty writes; reads checksum; O(1) pointer reset",
            ALLOC_COUNT,
            BLOCK_SIZE,
            [&]() -> std::uint64_t {
                std::uint64_t checksum = 0;
                std::vector<Transform64*> ptrs(ALLOC_COUNT);

                for (std::size_t i = 0; i < ALLOC_COUNT; ++i) {
                    auto* t = static_cast<Transform64*>(alloc.allocate(sizeof(Transform64), alignof(Transform64)));
                    t->pos[0] = static_cast<float>(i);
                    t->entity_id = static_cast<uint32_t>(i);
                    ptrs[i] = t;
                }

                for (std::size_t i = 0; i < ALLOC_COUNT; ++i) {
                    checksum += ptrs[i]->entity_id;
                }

                alloc.reset();
                return checksum;
            }
        );
    }
}

} // namespace volt::bench
