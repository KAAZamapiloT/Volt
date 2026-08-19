#pragma once

#include <algorithm>
#include <atomic>
#include <chrono>
#include <cstddef>
#include <cstdint>
#include <iomanip>
#include <iostream>
#include <numeric>
#include <span>
#include <stdexcept>
#include <string>
#include <thread>
#include <type_traits>
#include <utility>
#include <vector>

#include <volt/Containers/SPSCqueue.hpp>

namespace volt::tests {

using usize = std::size_t;
using Clock = std::chrono::steady_clock;

struct TestFailure : std::runtime_error {
    using std::runtime_error::runtime_error;
};

#define VOLT_SPSC_CHECK(expr)                                                     \
    do {                                                                          \
        if (!(expr)) {                                                            \
            throw ::volt::tests::TestFailure(                                     \
                std::string("CHECK failed: ") + #expr +                           \
                " at " + __FILE__ + ":" + std::to_string(__LINE__));             \
        }                                                                         \
    } while (false)

struct Timer {
    Clock::time_point start = Clock::now();

    double milliseconds() const noexcept {
        return std::chrono::duration<double, std::milli>(
            Clock::now() - start
        ).count();
    }
};

inline void banner(std::string_view title) {
    std::cout << "\n============================================================\n"
              << title << '\n'
              << "============================================================\n";
}

struct Tracked {
    static inline std::uint64_t constructions = 0;
    static inline std::uint64_t destructions = 0;
    static inline std::uint64_t copies = 0;
    static inline std::uint64_t moves = 0;

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

    static void reset() {
        constructions = 0;
        destructions = 0;
        copies = 0;
        moves = 0;
    }
};

struct MoveOnly {
    int value = 0;

    explicit MoveOnly(int v = 0) : value(v) {}

    MoveOnly(const MoveOnly&) = delete;
    MoveOnly& operator=(const MoveOnly&) = delete;

    MoveOnly(MoveOnly&& other) noexcept : value(other.value) {
        other.value = -1;
    }

    MoveOnly& operator=(MoveOnly&& other) noexcept {
        value = other.value;
        other.value = -1;
        return *this;
    }
};

inline void test_capacity_and_boundaries() {
    banner("SPSCQueue: capacity / empty / full");

    volt::SPSCQueue<int> q(8);

    VOLT_SPSC_CHECK(q.capacity() == 8);
    VOLT_SPSC_CHECK(q.empty());
    VOLT_SPSC_CHECK(!q.full());

    int out = -1;
    VOLT_SPSC_CHECK(!q.pop(out));

    for (int i = 0; i < 8; ++i) {
        VOLT_SPSC_CHECK(q.push(i));
    }

    VOLT_SPSC_CHECK(q.full());
    VOLT_SPSC_CHECK(q.size() == 8);
    VOLT_SPSC_CHECK(!q.push(999));

    for (int i = 0; i < 8; ++i) {
        VOLT_SPSC_CHECK(q.pop(out));
        VOLT_SPSC_CHECK(out == i);
    }

    VOLT_SPSC_CHECK(q.empty());
    VOLT_SPSC_CHECK(q.size() == 0);

    std::cout << "PASS\n";
}

inline void test_capacity_validation() {
    banner("SPSCQueue: power-of-two validation");

    bool threw = false;
    try {
        volt::SPSCQueue<int> invalid(12);
    } catch (...) {
        threw = true;
    }

    VOLT_SPSC_CHECK(threw);

    volt::SPSCQueue<int> q4(4);
    volt::SPSCQueue<int> q8(8);
    volt::SPSCQueue<int> q1024(1024);

    VOLT_SPSC_CHECK(q4.capacity() == 4);
    VOLT_SPSC_CHECK(q8.capacity() == 8);
    VOLT_SPSC_CHECK(q1024.capacity() == 1024);

    std::cout << "PASS\n";
}

inline void test_wraparound() {
    banner("SPSCQueue: wraparound");

    volt::SPSCQueue<int> q(4);

    constexpr int iterations = 100'000;

    for (int i = 0; i < iterations; ++i) {
        VOLT_SPSC_CHECK(q.push(i));

        int out = -1;
        VOLT_SPSC_CHECK(q.pop(out));
        VOLT_SPSC_CHECK(out == i);
    }

    VOLT_SPSC_CHECK(q.empty());

    std::cout << "PASS\n";
}

inline void test_lvalue_rvalue_emplace() {
    banner("SPSCQueue: lvalue / rvalue / emplace");

    volt::SPSCQueue<std::string> q(8);

    std::string a = "copy_me";
    std::string b = "move_me";

    VOLT_SPSC_CHECK(q.push(a));
    VOLT_SPSC_CHECK(q.push(std::move(b)));
    VOLT_SPSC_CHECK(q.emplace("hello"));
    VOLT_SPSC_CHECK(q.emplace(5, 'x'));

    std::string out;

    VOLT_SPSC_CHECK(q.pop(out));
    VOLT_SPSC_CHECK(out == "copy_me");

    VOLT_SPSC_CHECK(q.pop(out));
    VOLT_SPSC_CHECK(out == "move_me");

    VOLT_SPSC_CHECK(q.pop(out));
    VOLT_SPSC_CHECK(out == "hello");

    VOLT_SPSC_CHECK(q.pop(out));
    VOLT_SPSC_CHECK(out == "xxxxx");

    VOLT_SPSC_CHECK(q.empty());

    std::cout << "PASS\n";
}

inline void test_move_only() {
    banner("SPSCQueue: move-only type");

    volt::SPSCQueue<MoveOnly> q(8);

    VOLT_SPSC_CHECK(q.emplace(123));
    VOLT_SPSC_CHECK(q.emplace(456));

    MoveOnly out;

    VOLT_SPSC_CHECK(q.pop(out));
    VOLT_SPSC_CHECK(out.value == 123);

    VOLT_SPSC_CHECK(q.pop(out));
    VOLT_SPSC_CHECK(out.value == 456);

    VOLT_SPSC_CHECK(q.empty());

    std::cout << "PASS\n";
}

inline void test_lifetime() {
    banner("SPSCQueue: object lifetime");

    Tracked::reset();

    {
        volt::SPSCQueue<Tracked> q(8);

        for (int i = 0; i < 8; ++i) {
            VOLT_SPSC_CHECK(q.emplace(i));
        }

        for (int i = 0; i < 8; ++i) {
            Tracked out;
            VOLT_SPSC_CHECK(q.pop(out));
            VOLT_SPSC_CHECK(out.value == i);
        }

        VOLT_SPSC_CHECK(q.empty());
    }

    VOLT_SPSC_CHECK(Tracked::constructions == Tracked::destructions);

    std::cout << "PASS"
              << " | constructions=" << Tracked::constructions
              << " destructions=" << Tracked::destructions
              << " copies=" << Tracked::copies
              << " moves=" << Tracked::moves << '\n';
}

inline void test_bulk() {
    banner("SPSCQueue: bulk push / pop");

    volt::SPSCQueue<int> q(16);

    std::vector<int> input(16);
    std::iota(input.begin(), input.end(), 0);

    const usize pushed =
        q.push_bulk(std::span<const int>(input));

    VOLT_SPSC_CHECK(pushed == 16);
    VOLT_SPSC_CHECK(q.full());

    std::vector<int> output(16, -1);

    const usize popped =
        q.pop_bulk(std::span<int>(output));

    VOLT_SPSC_CHECK(popped == 16);
    VOLT_SPSC_CHECK(q.empty());
    VOLT_SPSC_CHECK(output == input);

    std::cout << "PASS\n";
}

inline void test_bulk_partial_and_wrap() {
    banner("SPSCQueue: partial bulk + wraparound");

    volt::SPSCQueue<int> q(8);

    std::vector<int> first(6);
    std::iota(first.begin(), first.end(), 100);

    VOLT_SPSC_CHECK(q.push_bulk(std::span<const int>(first)) == 6);

    std::vector<int> out1(4, -1);
    VOLT_SPSC_CHECK(q.pop_bulk(std::span<int>(out1)) == 4);

    for (usize i = 0; i < out1.size(); ++i) {
        VOLT_SPSC_CHECK(out1[i] == static_cast<int>(100 + i));
    }

    std::vector<int> second(6);
    std::iota(second.begin(), second.end(), 200);

    const usize pushed_second =
        q.push_bulk(std::span<const int>(second));

    VOLT_SPSC_CHECK(pushed_second == 6);

    std::vector<int> out2(8, -1);
    const usize popped =
        q.pop_bulk(std::span<int>(out2));

    // Remaining: 104,105, then 200..205
    VOLT_SPSC_CHECK(popped == 8);

    const std::vector<int> expected{
        104, 105, 200, 201, 202, 203, 204, 205
    };

    VOLT_SPSC_CHECK(out2 == expected);
    VOLT_SPSC_CHECK(q.empty());

    std::cout << "PASS\n";
}

struct StressResult {
    std::uint64_t produced = 0;
    std::uint64_t consumed = 0;
    std::uint64_t mismatches = 0;
    std::uint64_t checksum = 0;
    double milliseconds = 0.0;
};

inline StressResult run_order_stress(
    usize item_count,
    usize capacity
) {
    volt::SPSCQueue<std::uint64_t> q(capacity);

    std::atomic<bool> producer_done{false};
    std::atomic<std::uint64_t> produced{0};
    std::atomic<std::uint64_t> consumed{0};
    std::atomic<std::uint64_t> mismatches{0};

    Timer timer;

    std::thread producer([&] {
        for (std::uint64_t i = 0; i < item_count; ++i) {
            while (!q.push(i)) {
                std::this_thread::yield();
            }

            produced.fetch_add(1, std::memory_order_relaxed);
        }

        producer_done.store(true, std::memory_order_release);
    });

    std::thread consumer([&] {
        std::uint64_t expected = 0;
        std::uint64_t checksum = 0;

        while (expected < item_count) {
            std::uint64_t value = 0;

            if (!q.pop(value)) {
                if (!producer_done.load(std::memory_order_acquire)) {
                    std::this_thread::yield();
                }
                continue;
            }

            if (value != expected) {
                mismatches.fetch_add(1, std::memory_order_relaxed);
            }

            checksum += value;
            ++expected;
            consumed.fetch_add(1, std::memory_order_relaxed);
        }

        return checksum;
    });

    producer.join();
    consumer.join();

    // Recompute checksum from the deterministic sequence. This avoids
    // introducing another synchronization variable into the queue test.
    const std::uint64_t expected_checksum =
        (static_cast<std::uint64_t>(item_count - 1) *
         static_cast<std::uint64_t>(item_count)) / 2;

    return StressResult{
        produced.load(std::memory_order_relaxed),
        consumed.load(std::memory_order_relaxed),
        mismatches.load(std::memory_order_relaxed),
        expected_checksum,
        timer.milliseconds()
    };
}

inline void test_order_stress() {
    banner("SPSCQueue: 2M ordered producer/consumer stress");

    constexpr usize count = 2'000'000;

    const auto result = run_order_stress(count, 1024);

    const std::uint64_t expected_checksum =
        (static_cast<std::uint64_t>(count - 1) *
         static_cast<std::uint64_t>(count)) / 2;

    VOLT_SPSC_CHECK(result.produced == count);
    VOLT_SPSC_CHECK(result.consumed == count);
    VOLT_SPSC_CHECK(result.mismatches == 0);
    VOLT_SPSC_CHECK(result.checksum == expected_checksum);

    std::cout << "PASS"
              << " | produced=" << result.produced
              << " consumed=" << result.consumed
              << " mismatches=" << result.mismatches
              << " time=" << std::fixed << std::setprecision(3)
              << result.milliseconds << " ms\n";
}

inline void test_tiny_capacity_stress() {
    banner("SPSCQueue: tiny capacity stress");

    constexpr usize count = 500'000;

    for (usize capacity : {4u, 8u, 16u}) {
        const auto result = run_order_stress(count, capacity);

        VOLT_SPSC_CHECK(result.produced == count);
        VOLT_SPSC_CHECK(result.consumed == count);
        VOLT_SPSC_CHECK(result.mismatches == 0);

        std::cout << "capacity=" << capacity
                  << " time=" << std::fixed << std::setprecision(3)
                  << result.milliseconds << " ms\n";
    }

    std::cout << "PASS\n";
}

struct BenchResult {
    double milliseconds = 0.0;
    double million_items_per_second = 0.0;
};

inline BenchResult benchmark_scalar(
    usize item_count,
    usize capacity
) {
    volt::SPSCQueue<std::uint64_t> q(capacity);

    Timer timer;

    std::thread producer([&] {
        for (std::uint64_t i = 0; i < item_count; ++i) {
            while (!q.push(i)) {
                std::this_thread::yield();
            }
        }
    });

    std::thread consumer([&] {
        std::uint64_t expected = 0;

        while (expected < item_count) {
            std::uint64_t value = 0;

            if (!q.pop(value)) {
                std::this_thread::yield();
                continue;
            }

            if (value != expected) {
                std::abort();
            }

            ++expected;
        }
    });

    producer.join();
    consumer.join();

    const double ms = timer.milliseconds();
    const double sec = ms / 1000.0;

    return {
        ms,
        static_cast<double>(item_count) / sec / 1'000'000.0
    };
}

inline BenchResult benchmark_bulk(
    usize item_count,
    usize capacity,
    usize batch_size
) {
    volt::SPSCQueue<std::uint64_t> q(capacity);

    std::vector<std::uint64_t> input(batch_size);
    std::vector<std::uint64_t> output(batch_size);

    Timer timer;

    std::thread producer([&] {
        std::uint64_t base = 0;

        while (base < item_count) {
            const usize count = static_cast<usize>(
                std::min<std::uint64_t>(
                    batch_size,
                    item_count - base
                )
            );

            for (usize i = 0; i < count; ++i) {
                input[i] = base + i;
            }

            usize pushed = 0;

            while (pushed < count) {
                const usize n = q.push_bulk(
                    std::span<const std::uint64_t>(
                        input.data() + pushed,
                        count - pushed
                    )
                );

                pushed += n;

                if (n == 0) {
                    std::this_thread::yield();
                }
            }

            base += count;
        }
    });

    std::thread consumer([&] {
        std::uint64_t expected = 0;

        while (expected < item_count) {
            const usize remaining =
                static_cast<usize>(item_count - expected);

            const usize count = std::min(batch_size, remaining);

            usize popped = 0;

            while (popped < count) {
                const usize n = q.pop_bulk(
                    std::span<std::uint64_t>(
                        output.data() + popped,
                        count - popped
                    )
                );

                popped += n;

                if (n == 0) {
                    std::this_thread::yield();
                }
            }

            for (usize i = 0; i < count; ++i) {
                if (output[i] != expected + i) {
                    std::abort();
                }
            }

            expected += count;
        }
    });

    producer.join();
    consumer.join();

    const double ms = timer.milliseconds();
    const double sec = ms / 1000.0;

    return {
        ms,
        static_cast<double>(item_count) / sec / 1'000'000.0
    };
}

inline void run_benchmarks() {
    banner("SPSCQueue: throughput benchmarks");

    constexpr usize item_count = 10'000'000;

    for (usize capacity : {64u, 1024u, 16'384u}) {
        std::cout << "\nCapacity: " << capacity << '\n';

        const auto scalar = benchmark_scalar(item_count, capacity);

        std::cout << "scalar       : "
                  << std::fixed << std::setprecision(3)
                  << scalar.milliseconds << " ms | "
                  << std::setprecision(2)
                  << scalar.million_items_per_second
                  << " M items/s\n";

        for (usize batch : {8u, 32u}) {
            const auto bulk =
                benchmark_bulk(item_count, capacity, batch);

            std::cout << "bulk(" << std::setw(2) << batch << ")   : "
                      << std::fixed << std::setprecision(3)
                      << bulk.milliseconds << " ms | "
                      << std::setprecision(2)
                      << bulk.million_items_per_second
                      << " M items/s\n";
        }
    }

    std::cout << "\nRun benchmarks in Release/RelWithDebInfo, not Debug.\n";
}

inline int run_spsc_suite() {
    try {
        banner("Volt SPSCQueue test suite");

        test_capacity_validation();
        test_capacity_and_boundaries();
        test_wraparound();
        test_lvalue_rvalue_emplace();
        test_move_only();
        test_lifetime();

        test_bulk();
        test_bulk_partial_and_wrap();

        test_order_stress();
        test_tiny_capacity_stress();

        run_benchmarks();

        std::cout << "\nALL SPSC TESTS PASSED\n";
        return 0;
    }
    catch (const TestFailure& e) {
        std::cerr << "\nTEST FAILURE: " << e.what() << '\n';
        return 1;
    }
    catch (const std::exception& e) {
        std::cerr << "\nUNEXPECTED EXCEPTION: " << e.what() << '\n';
        return 2;
    }
    catch (...) {
        std::cerr << "\nUNKNOWN EXCEPTION\n";
        return 3;
    }
}

#undef VOLT_SPSC_CHECK

} // namespace volt::tests

#ifdef VOLT_SPSC_TEST_MAIN
int main() {
    return volt::tests::run_spsc_suite();
}
#endif
