#pragma once

#include <BenchmarkHarness.hpp>
#include <volt/Containers/SmallVector.hpp>
#include <vector>

namespace volt::bench {

inline void run_smallvector_benchmarks() {
    constexpr std::size_t ITERATIONS = 50'000;
    constexpr std::size_t SMALL_COUNT = 8; // fits completely in SmallVector<int, 16> inline buffer
    constexpr std::size_t OVERFLOW_COUNT = 48; // spills past 16 to heap

    // ------------------------------------------------------------------------
    // Scenario 1: Small Collection (N=8) - Stack SBO vs Heap Allocation
    // ------------------------------------------------------------------------
    BenchmarkSuite::instance().measure(
        "SmallVector",
        "std::vector (N=8 small, baseline)",
        "Every instance allocates dynamic heap block; writes & checksums items",
        ITERATIONS * SMALL_COUNT,
        sizeof(int),
        [&]() -> std::uint64_t {
            std::uint64_t checksum = 0;
            for (std::size_t iter = 0; iter < ITERATIONS; ++iter) {
                std::vector<int> v;
                for (std::size_t i = 0; i < SMALL_COUNT; ++i) {
                    v.push_back(static_cast<int>(i + iter));
                }
                checksum += v[0];
            }
            return checksum;
        }
    );

    BenchmarkSuite::instance().measure(
        "SmallVector",
        "volt::SmallVector<int, 16> (N=8 SBO)",
        "Zero heap allocations; 100% stack buffer; writes & checksums items",
        ITERATIONS * SMALL_COUNT,
        sizeof(int),
        [&]() -> std::uint64_t {
            std::uint64_t checksum = 0;
            for (std::size_t iter = 0; iter < ITERATIONS; ++iter) {
                volt::SmallVector<int, 16> v;
                for (std::size_t i = 0; i < SMALL_COUNT; ++i) {
                    v.push_back(static_cast<int>(i + iter));
                }
                checksum += v[0];
            }
            return checksum;
        }
    );

    // ------------------------------------------------------------------------
    // Scenario 2: Overflow Spill (N=48) - Growth Past Inline Buffer
    // ------------------------------------------------------------------------
    BenchmarkSuite::instance().measure(
        "SmallVector",
        "std::vector (N=48 growth, baseline)",
        "Grows dynamically from 0 to 48 items via heap reallocations",
        ITERATIONS * OVERFLOW_COUNT,
        sizeof(int),
        [&]() -> std::uint64_t {
            std::uint64_t checksum = 0;
            for (std::size_t iter = 0; iter < ITERATIONS; ++iter) {
                std::vector<int> v;
                for (std::size_t i = 0; i < OVERFLOW_COUNT; ++i) {
                    v.push_back(static_cast<int>(i + iter));
                }
                checksum += v[0];
            }
            return checksum;
        }
    );

    BenchmarkSuite::instance().measure(
        "SmallVector",
        "volt::SmallVector<int, 16> (N=48 spill to heap)",
        "Starts on stack, overflows past 16 items and reallocates to heap buffer",
        ITERATIONS * OVERFLOW_COUNT,
        sizeof(int),
        [&]() -> std::uint64_t {
            std::uint64_t checksum = 0;
            for (std::size_t iter = 0; iter < ITERATIONS; ++iter) {
                volt::SmallVector<int, 16> v;
                for (std::size_t i = 0; i < OVERFLOW_COUNT; ++i) {
                    v.push_back(static_cast<int>(i + iter));
                }
                checksum += v[0];
            }
            return checksum;
        }
    );
}

} // namespace volt::bench
