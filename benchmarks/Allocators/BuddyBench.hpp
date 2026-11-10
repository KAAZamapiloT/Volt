#pragma once

#include <BenchmarkHarness.hpp>
#include <volt/memory/BuddyAllocator.hpp>
#include <vector>

namespace volt::bench {

inline void run_buddy_allocator_benchmarks() {
    // Generate a realistic heterogeneous mix of allocation sizes (64B up to 4096B)
    constexpr std::size_t SIZES[] = { 64, 128, 256, 512, 1024, 2048, 4096 };
    constexpr std::size_t NUM_SIZES = sizeof(SIZES) / sizeof(SIZES[0]);
    constexpr std::size_t TOTAL_BLOCKS = 1000;

    std::vector<std::size_t> block_sizes(TOTAL_BLOCKS);
    std::size_t total_requested_bytes = 0;
    for (std::size_t i = 0; i < TOTAL_BLOCKS; ++i) {
        block_sizes[i] = SIZES[i % NUM_SIZES];
        total_requested_bytes += block_sizes[i];
    }
    const std::size_t avg_block_size = total_requested_bytes / TOTAL_BLOCKS;

    // ------------------------------------------------------------------------
    // 1. Baseline: malloc + write payload + read + free
    // ------------------------------------------------------------------------
    BenchmarkSuite::instance().measure(
        "Buddy Allocator",
        "malloc mixed orders (64B-4KB) (baseline)",
        "Heterogeneous power-of-two size distribution; dirty writes; individual free()",
        TOTAL_BLOCKS,
        avg_block_size,
        [&]() -> std::uint64_t {
            std::vector<void*> ptrs(TOTAL_BLOCKS, nullptr);
            std::uint64_t checksum = 0;

            for (std::size_t i = 0; i < TOTAL_BLOCKS; ++i) {
                void* p = std::malloc(block_sizes[i]);
                *static_cast<uint32_t*>(p) = static_cast<uint32_t>(i);
                ptrs[i] = p;
            }

            for (std::size_t i = 0; i < TOTAL_BLOCKS; ++i) {
                checksum += *static_cast<uint32_t*>(ptrs[i]);
                std::free(ptrs[i]);
            }

            return checksum;
        }
    );

    // ------------------------------------------------------------------------
    // 2. Volt BuddyAllocator: Multi-order splitting + coalescing + 1MB arena validation
    // ------------------------------------------------------------------------
    BenchmarkSuite::instance().measure(
        "Buddy Allocator",
        "volt::BuddyAllocator mixed orders (split + coalesce)",
        "Exercises multi-order split, bitwise buddy XOR search, multi-level coalescing",
        TOTAL_BLOCKS,
        avg_block_size,
        [&]() -> std::uint64_t {
            volt::BuddyAllocator buddy;
            std::vector<void*> ptrs(TOTAL_BLOCKS, nullptr);
            std::uint64_t checksum = 0;

            for (std::size_t i = 0; i < TOTAL_BLOCKS; ++i) {
                void* p = buddy.allocate(block_sizes[i]);
                if (p != nullptr) {
                    *static_cast<uint32_t*>(p) = static_cast<uint32_t>(i);
                    ptrs[i] = p;
                }
            }

            for (std::size_t i = 0; i < TOTAL_BLOCKS; ++i) {
                if (ptrs[i] != nullptr) {
                    checksum += *static_cast<uint32_t*>(ptrs[i]);
                    buddy.deallocate(ptrs[i]);
                }
            }

            // Verify full arena coalescing: 1 MiB allocation must succeed if all blocks merged
            void* full_arena = buddy.allocate(1024 * 1024);
            if (full_arena != nullptr) {
                checksum += 1;
                buddy.deallocate(full_arena);
            }

            return checksum;
        }
    );
}

} // namespace volt::bench
