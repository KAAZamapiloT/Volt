#pragma once

#include <algorithm>
#include <array>
#include <atomic>
#include <bit>
#include <chrono>
#include <cstddef>
#include <cstdint>
#include <iomanip>
#include <iostream>
#include <memory>
#include <random>
#include <stdexcept>
#include <string>
#include <utility>
#include <vector>

#include <volt/memory/PoolAllocator.hpp>

namespace volt::tests {

using usize = std::size_t;
using Clock = std::chrono::steady_clock;

struct PoolTestFailure : std::runtime_error {
    using std::runtime_error::runtime_error;
};
struct Timer {
    using Clock = std::chrono::steady_clock;

    Clock::time_point start = Clock::now();

    double milliseconds() const noexcept {
        return std::chrono::duration<double, std::milli>(
            Clock::now() - start
        ).count();
    }
};

#define VOLT_POOL_CHECK(expr)                                                \
    do {                                                                      \
        if (!(expr)) {                                                        \
            throw ::volt::tests::PoolTestFailure(                             \
                std::string("CHECK failed: ") + #expr +                      \
                " at " + __FILE__ + ":" + std::to_string(__LINE__));         \
        }                                                                     \
    } while (false)

inline void pool_banner(const char* title) {
    std::cout
        << "\n============================================================\n"
        << title << '\n'
        << "============================================================\n";
}

struct Tracked {
    static inline std::uint64_t constructions = 0;
    static inline std::uint64_t destructions = 0;
    static inline std::uint64_t moves = 0;
    static inline std::uint64_t copies = 0;

    int value = 0;

    explicit Tracked(int v = 0) : value(v) {
        ++constructions;
    }

    Tracked(const Tracked& other) : value(other.value) {
        ++constructions;
        ++copies;
    }

    Tracked(Tracked&& other) noexcept : value(other.value) {
        other.value = -1;
        ++constructions;
        ++moves;
    }

    Tracked& operator=(const Tracked& other) {
        value = other.value;
        ++copies;
        return *this;
    }

    Tracked& operator=(Tracked&& other) noexcept {
        value = other.value;
        other.value = -1;
        ++moves;
        return *this;
    }

    ~Tracked() {
        ++destructions;
    }

    static void reset() noexcept {
        constructions = 0;
        destructions = 0;
        moves = 0;
        copies = 0;
    }
};

struct ThrowingType {
    explicit ThrowingType(int value) {
        if (value < 0) {
            throw std::runtime_error("intentional construction failure");
        }
    }
};

struct alignas(64) OverAligned {
    std::uint64_t values[8]{};
};

// ------------------------------------------------------------
// Basic capacity / state
// ------------------------------------------------------------

inline void test_pool_capacity_and_state() {
    pool_banner("PoolAllocator: capacity / state");

    volt::PoolAllocator<std::uint64_t, 8> pool;

    VOLT_POOL_CHECK(pool.capacity() == 8);
    VOLT_POOL_CHECK(pool.available() == 8);
    VOLT_POOL_CHECK(pool.size() == 0);
    VOLT_POOL_CHECK(pool.empty());
    VOLT_POOL_CHECK(!pool.full());

    std::array<std::uint64_t*, 8> objects{};

    for (usize i = 0; i < objects.size(); ++i) {
        objects[i] = pool.allocate();

        VOLT_POOL_CHECK(objects[i] != nullptr);
        VOLT_POOL_CHECK(pool.size() == i + 1);
        VOLT_POOL_CHECK(pool.available() == 8 - i - 1);
    }

    VOLT_POOL_CHECK(pool.full());
    VOLT_POOL_CHECK(pool.allocate() == nullptr);

    for (auto* ptr : objects) {
        pool.deallocate(ptr);
    }

    VOLT_POOL_CHECK(pool.empty());
    VOLT_POOL_CHECK(pool.available() == 8);

    std::cout << "PASS\n";
}

// ------------------------------------------------------------
// Free-list reuse
// ------------------------------------------------------------

inline void test_pool_slot_reuse() {
    pool_banner("PoolAllocator: slot reuse / free-list");

    volt::PoolAllocator<std::uint64_t, 8> pool;

    std::array<std::uint64_t*, 8> objects{};

    for (usize i = 0; i < objects.size(); ++i) {
        objects[i] = pool.allocate();
        VOLT_POOL_CHECK(objects[i] != nullptr);
    }

    pool.deallocate(objects[1]);
    pool.deallocate(objects[3]);
    pool.deallocate(objects[5]);

    // Free-list is LIFO.
    auto* a = pool.allocate();
    auto* b = pool.allocate();
    auto* c = pool.allocate();

    VOLT_POOL_CHECK(a == objects[5]);
    VOLT_POOL_CHECK(b == objects[3]);
    VOLT_POOL_CHECK(c == objects[1]);

    // Now all original slots are allocated again.
    for (auto* ptr : objects) {
        pool.deallocate(ptr);
    }

    VOLT_POOL_CHECK(pool.empty());
    VOLT_POOL_CHECK(pool.available() == pool.capacity());

    std::cout << "PASS\n";
}

// ------------------------------------------------------------
// Create/destroy + object lifetime
// ------------------------------------------------------------

inline void test_pool_create_destroy() {
    pool_banner("PoolAllocator: create / destroy / lifetime");

    Tracked::reset();

    {
        volt::PoolAllocator<Tracked, 16> pool;

        std::array<Tracked*, 16> objects{};

        for (usize i = 0; i < objects.size(); ++i) {
            objects[i] = pool.create(static_cast<int>(i));

            VOLT_POOL_CHECK(objects[i] != nullptr);
            VOLT_POOL_CHECK(
                objects[i]->value == static_cast<int>(i)
            );
        }

        VOLT_POOL_CHECK(pool.full());
        VOLT_POOL_CHECK(Tracked::constructions == 16);

        for (usize i = 0; i < objects.size(); ++i) {
            pool.destroy(objects[i]);
        }

        VOLT_POOL_CHECK(pool.empty());
        VOLT_POOL_CHECK(Tracked::destructions == 16);
    }

    VOLT_POOL_CHECK(
        Tracked::constructions == Tracked::destructions
    );

    std::cout
        << "PASS"
        << " | constructions=" << Tracked::constructions
        << " destructions=" << Tracked::destructions
        << '\n';
}

// ------------------------------------------------------------
// create() exception rollback
// ------------------------------------------------------------

inline void test_pool_create_exception_rollback() {
    pool_banner("PoolAllocator: create exception rollback");

    volt::PoolAllocator<ThrowingType, 4> pool;

    bool threw = false;

    try {
        (void)pool.create(-1);
    }
    catch (const std::exception&) {
        threw = true;
    }

    VOLT_POOL_CHECK(threw);
    VOLT_POOL_CHECK(pool.size() == 0);
    VOLT_POOL_CHECK(pool.available() == 4);

    auto* ptr = pool.create(42);

    VOLT_POOL_CHECK(ptr != nullptr);
    VOLT_POOL_CHECK(pool.size() == 1);

    pool.destroy(ptr);

    VOLT_POOL_CHECK(pool.empty());

    std::cout << "PASS\n";
}

// ------------------------------------------------------------
// Alignment
// ------------------------------------------------------------

inline void test_pool_alignment() {
    pool_banner("PoolAllocator: alignment");

    volt::PoolAllocator<OverAligned, 32> pool;

    std::array<OverAligned*, 32> objects{};

    for (usize i = 0; i < objects.size(); ++i) {
        objects[i] = pool.allocate();

        VOLT_POOL_CHECK(objects[i] != nullptr);

        const auto address =
            reinterpret_cast<std::uintptr_t>(objects[i]);

        VOLT_POOL_CHECK(
            address % alignof(OverAligned) == 0
        );
    }

    for (auto* ptr : objects) {
        pool.deallocate(ptr);
    }

    VOLT_POOL_CHECK(pool.empty());

    std::cout << "PASS\n";
}

// ------------------------------------------------------------
// Distinct addresses
// ------------------------------------------------------------

inline void test_pool_addresses_are_distinct() {
    pool_banner("PoolAllocator: distinct slot addresses");

    volt::PoolAllocator<std::uint64_t, 32> pool;

    std::array<std::uint64_t*, 32> objects{};

    for (usize i = 0; i < objects.size(); ++i) {
        objects[i] = pool.allocate();
        VOLT_POOL_CHECK(objects[i] != nullptr);
    }

    for (usize i = 0; i < objects.size(); ++i) {
        for (usize j = i + 1; j < objects.size(); ++j) {
            VOLT_POOL_CHECK(objects[i] != objects[j]);
        }
    }

    for (auto* ptr : objects) {
        pool.deallocate(ptr);
    }

    std::cout << "PASS\n";
}

// ------------------------------------------------------------
// Randomized allocate/deallocate order
// ------------------------------------------------------------

inline void test_pool_randomized_reuse() {
    pool_banner("PoolAllocator: randomized allocate/deallocate");

    constexpr usize Capacity = 1024;
    constexpr usize Operations = 200'000;

    volt::PoolAllocator<std::uint64_t, Capacity> pool;

    std::vector<std::uint64_t*> live;
    live.reserve(Capacity);

    std::mt19937_64 rng(0xC0FFEE);

    for (usize op = 0; op < Operations; ++op) {
        const bool should_allocate =
            live.empty() ||
            (live.size() < Capacity && ((rng() & 1ULL) != 0));

        if (should_allocate) {
            auto* ptr = pool.allocate();

            VOLT_POOL_CHECK(ptr != nullptr);

            *ptr = static_cast<std::uint64_t>(op);
            live.push_back(ptr);
        }
        else {
            const usize index =
                static_cast<usize>(rng() % live.size());

            auto* ptr = live[index];

            pool.deallocate(ptr);

            live[index] = live.back();
            live.pop_back();
        }

        VOLT_POOL_CHECK(pool.size() == live.size());
        VOLT_POOL_CHECK(
            pool.available() + pool.size() == Capacity
        );
    }

    for (auto* ptr : live) {
        pool.deallocate(ptr);
    }

    VOLT_POOL_CHECK(pool.empty());
    VOLT_POOL_CHECK(pool.available() == Capacity);

    std::cout << "PASS\n";
}

// ------------------------------------------------------------
// Reuse stress
// ------------------------------------------------------------

inline void test_pool_reuse_stress() {
    pool_banner("PoolAllocator: repeated reuse stress");

    constexpr usize Capacity = 1024;
    constexpr usize Iterations = 2'000'000;

    volt::PoolAllocator<std::uint64_t, Capacity> pool;

    for (usize i = 0; i < Iterations; ++i) {
        auto* ptr = pool.allocate();

        VOLT_POOL_CHECK(ptr != nullptr);

        *ptr = static_cast<std::uint64_t>(i);
        VOLT_POOL_CHECK(*ptr == i);

        pool.deallocate(ptr);

        VOLT_POOL_CHECK(pool.empty());
        VOLT_POOL_CHECK(pool.available() == Capacity);
    }

    std::cout << "PASS\n";
}

// ------------------------------------------------------------
// Timing
// ------------------------------------------------------------

struct BenchResult {
    double milliseconds = 0.0;
    double million_operations_per_second = 0.0;
    double nanoseconds_per_operation = 0.0;
};

inline void benchmark_escape(std::uintptr_t value) noexcept {
#if defined(_MSC_VER)
    _ReadWriteBarrier();
    (void)value;
#elif defined(__GNUC__) || defined(__clang__)
    asm volatile("" : : "r,m"(value) : "memory");
#else
    volatile auto sink = value;
    (void)sink;
#endif
}

inline BenchResult benchmark_pool_allocate_deallocate(
    usize operations
) {
    volt::PoolAllocator<std::uint64_t, 1024> pool;

    // Warm up lazy free-list initialization.
    std::array<std::uint64_t*, 1024> warmup{};

    for (usize i = 0; i < warmup.size(); ++i) {
        warmup[i] = pool.allocate();
    }

    for (auto* ptr : warmup) {
        pool.deallocate(ptr);
    }

    Timer timer;
    std::uintptr_t sink = 0;

    for (usize i = 0; i < operations; ++i) {
        auto* ptr = pool.allocate();

        sink ^= reinterpret_cast<std::uintptr_t>(ptr);

        pool.deallocate(ptr);
    }

    benchmark_escape(sink);

    const double ms = timer.milliseconds();
    const double seconds = ms / 1000.0;

    return {
        ms,
        static_cast<double>(operations) / seconds / 1'000'000.0,
        (ms * 1'000'000.0) / static_cast<double>(operations)
    };
}

inline BenchResult benchmark_pool_create_destroy(
    usize operations
) {
    volt::PoolAllocator<std::uint64_t, 1024> pool;

    // Warm up lazy free-list initialization.
    std::array<std::uint64_t*, 1024> warmup{};

    for (usize i = 0; i < warmup.size(); ++i) {
        warmup[i] = pool.create(i);
    }

    for (auto* ptr : warmup) {
        pool.destroy(ptr);
    }

    Timer timer;
    std::uintptr_t sink = 0;

    for (usize i = 0; i < operations; ++i) {
        auto* ptr = pool.create(
            static_cast<std::uint64_t>(i)
        );

        sink ^= reinterpret_cast<std::uintptr_t>(ptr);

        pool.destroy(ptr);
    }

    benchmark_escape(sink);

    const double ms = timer.milliseconds();
    const double seconds = ms / 1000.0;

    return {
        ms,
        static_cast<double>(operations) / seconds / 1'000'000.0,
        (ms * 1'000'000.0) / static_cast<double>(operations)
    };
}

inline BenchResult benchmark_new_delete(
    usize operations
) {
    Timer timer;
    std::uintptr_t sink = 0;

    for (usize i = 0; i < operations; ++i) {
        auto* ptr =
            new std::uint64_t(
                static_cast<std::uint64_t>(i)
            );

        sink ^= reinterpret_cast<std::uintptr_t>(ptr);

        delete ptr;
    }

    benchmark_escape(sink);

    const double ms = timer.milliseconds();
    const double seconds = ms / 1000.0;

    return {
        ms,
        static_cast<double>(operations) / seconds / 1'000'000.0,
        (ms * 1'000'000.0) / static_cast<double>(operations)
    };
}

inline void benchmark_pool_vs_new() {
    pool_banner("PoolAllocator: timing benchmark");

    constexpr usize Operations = 5'000'000;

    const auto pool =
        benchmark_pool_allocate_deallocate(Operations);

    const auto create_destroy =
        benchmark_pool_create_destroy(Operations);

    const auto heap =
        benchmark_new_delete(Operations);

    std::cout
        << std::fixed << std::setprecision(3)

        << "operations                 : "
        << Operations << '\n'

        << "pool allocate/deallocate   : "
        << pool.milliseconds << " ms | "
        << pool.million_operations_per_second
        << " M ops/s | "
        << pool.nanoseconds_per_operation
        << " ns/op\n"

        << "pool create/destroy       : "
        << create_destroy.milliseconds << " ms | "
        << create_destroy.million_operations_per_second
        << " M ops/s | "
        << create_destroy.nanoseconds_per_operation
        << " ns/op\n"

        << "new/delete                : "
        << heap.milliseconds << " ms | "
        << heap.million_operations_per_second
        << " M ops/s | "
        << heap.nanoseconds_per_operation
        << " ns/op\n";

    if (pool.milliseconds > 0.0) {
        std::cout
            << "new/delete vs pool speedup: "
            << heap.milliseconds / pool.milliseconds
            << "x\n";
    }

    std::cout
        << "\nRun this benchmark in Release or RelWithDebInfo.\n"
        << "Debug timings are useful for diagnostics, not performance claims.\n";
}

// ------------------------------------------------------------
// Full suite
// ------------------------------------------------------------

inline int run_pool_allocator_suite() {
    try {
        pool_banner("Volt PoolAllocator test suite");

        test_pool_capacity_and_state();
        test_pool_slot_reuse();
        test_pool_create_destroy();
        test_pool_create_exception_rollback();
        test_pool_alignment();
        test_pool_addresses_are_distinct();
        test_pool_randomized_reuse();
        test_pool_reuse_stress();

        benchmark_pool_vs_new();

        std::cout
            << "\nALL POOL ALLOCATOR TESTS PASSED\n";

        return 0;
    }
    catch (const PoolTestFailure& e) {
        std::cerr
            << "\nTEST FAILURE: "
            << e.what() << '\n';

        return 1;
    }
    catch (const std::exception& e) {
        std::cerr
            << "\nUNEXPECTED EXCEPTION: "
            << e.what() << '\n';

        return 2;
    }
    catch (...) {
        std::cerr
            << "\nUNKNOWN EXCEPTION\n";

        return 3;
    }
}

#undef VOLT_POOL_CHECK

} // namespace volt::tests

#ifdef VOLT_POOL_ALLOCATOR_TEST_MAIN

int main() {
    return volt::tests::run_pool_allocator_suite();
}

#endif
