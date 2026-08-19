#pragma once

#include <atomic>
#include <chrono>
#include <cstddef>
#include <cstdint>
#include <iostream>
#include <numeric>
#include <span>
#include <string>
#include <thread>
#include <vector>

#include <volt/Containers/SPSCQueueStatic.hpp>

namespace volt::tests {

    using usize = std::size_t;
    using Clock = std::chrono::steady_clock;

    struct StaticTestFailure : std::runtime_error {
        using std::runtime_error::runtime_error;
    };

#define VOLT_STATIC_SPSC_CHECK(expr)                                      \
    do {                                                                  \
        if (!(expr)) {                                                    \
            throw ::volt::tests::StaticTestFailure(                      \
                std::string("CHECK failed: ") + #expr +                  \
                " at " + __FILE__ + ":" + std::to_string(__LINE__));     \
        }                                                                 \
    } while (false)

    inline void static_banner(std::string_view title) {
        std::cout << "\n============================================================\n"
            << title << '\n'
            << "============================================================\n";
    }


    // ============================================================
    // Basic capacity / empty / full
    // ============================================================

    inline void test_static_capacity_and_boundaries() {
        static_banner("Static SPSCQueue: capacity / empty / full");

        volt::SPSCQueueStatic<int, 8> q;

        VOLT_STATIC_SPSC_CHECK(q.static_capacity == 8);
        VOLT_STATIC_SPSC_CHECK(q.capacity() == 8);
        VOLT_STATIC_SPSC_CHECK(q.empty());
        VOLT_STATIC_SPSC_CHECK(!q.full());
        VOLT_STATIC_SPSC_CHECK(q.size() == 0);

        int out = -1;

        VOLT_STATIC_SPSC_CHECK(!q.pop(out));

        for (int i = 0; i < 8; ++i) {
            VOLT_STATIC_SPSC_CHECK(q.push(i));
        }

        VOLT_STATIC_SPSC_CHECK(q.full());
        VOLT_STATIC_SPSC_CHECK(q.size() == 8);
        VOLT_STATIC_SPSC_CHECK(!q.push(999));

        for (int i = 0; i < 8; ++i) {
            VOLT_STATIC_SPSC_CHECK(q.pop(out));
            VOLT_STATIC_SPSC_CHECK(out == i);
        }

        VOLT_STATIC_SPSC_CHECK(q.empty());
        VOLT_STATIC_SPSC_CHECK(q.size() == 0);

        std::cout << "PASS\n";
    }


    // ============================================================
    // Wraparound
    // ============================================================

    inline void test_static_wraparound() {
        static_banner("Static SPSCQueue: wraparound");

        volt::SPSCQueueStatic<int, 4> q;

        constexpr int iterations = 100'000;

        for (int i = 0; i < iterations; ++i) {
            VOLT_STATIC_SPSC_CHECK(q.push(i));

            int out = -1;
            VOLT_STATIC_SPSC_CHECK(q.pop(out));
            VOLT_STATIC_SPSC_CHECK(out == i);
        }

        VOLT_STATIC_SPSC_CHECK(q.empty());

        std::cout << "PASS\n";
    }


    // ============================================================
    // lvalue / rvalue / emplace
    // ============================================================

    inline void test_static_lvalue_rvalue_emplace() {
        static_banner("Static SPSCQueue: lvalue / rvalue / emplace");

        volt::SPSCQueueStatic<std::string, 8> q;

        std::string a = "copy_me";
        std::string b = "move_me";

        VOLT_STATIC_SPSC_CHECK(q.push(a));
        VOLT_STATIC_SPSC_CHECK(q.push(std::move(b)));
        VOLT_STATIC_SPSC_CHECK(q.emplace("hello"));
        VOLT_STATIC_SPSC_CHECK(q.emplace(5, 'x'));

        std::string out;

        VOLT_STATIC_SPSC_CHECK(q.pop(out));
        VOLT_STATIC_SPSC_CHECK(out == "copy_me");

        VOLT_STATIC_SPSC_CHECK(q.pop(out));
        VOLT_STATIC_SPSC_CHECK(out == "move_me");

        VOLT_STATIC_SPSC_CHECK(q.pop(out));
        VOLT_STATIC_SPSC_CHECK(out == "hello");

        VOLT_STATIC_SPSC_CHECK(q.pop(out));
        VOLT_STATIC_SPSC_CHECK(out == "xxxxx");

        VOLT_STATIC_SPSC_CHECK(q.empty());

        std::cout << "PASS\n";
    }


    // ============================================================
    // Move-only
    // ============================================================

    struct StaticMoveOnly {
        int value = 0;

        explicit StaticMoveOnly(int v = 0)
            : value(v) {
        }

        StaticMoveOnly(const StaticMoveOnly&) = delete;
        StaticMoveOnly& operator=(const StaticMoveOnly&) = delete;

        StaticMoveOnly(StaticMoveOnly&& other) noexcept
            : value(other.value) {
            other.value = -1;
        }

        StaticMoveOnly& operator=(StaticMoveOnly&& other) noexcept {
            value = other.value;
            other.value = -1;
            return *this;
        }
    };

    inline void test_static_move_only() {
        static_banner("Static SPSCQueue: move-only type");

        volt::SPSCQueueStatic<StaticMoveOnly, 8> q;

        VOLT_STATIC_SPSC_CHECK(q.emplace(123));
        VOLT_STATIC_SPSC_CHECK(q.emplace(456));

        StaticMoveOnly out;

        VOLT_STATIC_SPSC_CHECK(q.pop(out));
        VOLT_STATIC_SPSC_CHECK(out.value == 123);

        VOLT_STATIC_SPSC_CHECK(q.pop(out));
        VOLT_STATIC_SPSC_CHECK(out.value == 456);

        VOLT_STATIC_SPSC_CHECK(q.empty());

        std::cout << "PASS\n";
    }


    // ============================================================
    // Lifetime
    // ============================================================

    struct StaticTracked {
        static inline std::uint64_t constructions = 0;
        static inline std::uint64_t destructions = 0;
        static inline std::uint64_t copies = 0;
        static inline std::uint64_t moves = 0;

        int value = 0;

        explicit StaticTracked(int v = 0)
            : value(v) {
            ++constructions;
        }

        StaticTracked(const StaticTracked& other)
            : value(other.value) {
            ++constructions;
            ++copies;
        }

        StaticTracked(StaticTracked&& other) noexcept
            : value(other.value) {
            other.value = -1;
            ++constructions;
            ++moves;
        }

        StaticTracked& operator=(const StaticTracked& other) {
            value = other.value;
            ++copies;
            return *this;
        }

        StaticTracked& operator=(StaticTracked&& other) noexcept {
            value = other.value;
            other.value = -1;
            ++moves;
            return *this;
        }

        ~StaticTracked() {
            ++destructions;
        }

        static void reset() {
            constructions = 0;
            destructions = 0;
            copies = 0;
            moves = 0;
        }
    };

    inline void test_static_lifetime() {
        static_banner("Static SPSCQueue: object lifetime");

        StaticTracked::reset();

        {
            volt::SPSCQueueStatic<StaticTracked, 8> q;

            for (int i = 0; i < 8; ++i) {
                VOLT_STATIC_SPSC_CHECK(q.emplace(i));
            }

            for (int i = 0; i < 8; ++i) {
                StaticTracked out;
                VOLT_STATIC_SPSC_CHECK(q.pop(out));
                VOLT_STATIC_SPSC_CHECK(out.value == i);
            }

            VOLT_STATIC_SPSC_CHECK(q.empty());
        }

        VOLT_STATIC_SPSC_CHECK(
            StaticTracked::constructions == StaticTracked::destructions
        );

        std::cout << "PASS"
            << " | constructions=" << StaticTracked::constructions
            << " destructions=" << StaticTracked::destructions
            << " copies=" << StaticTracked::copies
            << " moves=" << StaticTracked::moves << '\n';
    }


    // ============================================================
    // Bulk
    // ============================================================

    inline void test_static_bulk() {
        static_banner("Static SPSCQueue: bulk push / pop");

        volt::SPSCQueueStatic<int, 16> q;

        std::vector<int> input(16);
        std::iota(input.begin(), input.end(), 0);

        const usize pushed =
            q.push_bulk(std::span<const int>(input));

        VOLT_STATIC_SPSC_CHECK(pushed == 16);
        VOLT_STATIC_SPSC_CHECK(q.full());

        std::vector<int> output(16, -1);

        const usize popped =
            q.pop_bulk(std::span<int>(output));

        VOLT_STATIC_SPSC_CHECK(popped == 16);
        VOLT_STATIC_SPSC_CHECK(q.empty());
        VOLT_STATIC_SPSC_CHECK(output == input);

        std::cout << "PASS\n";
    }


    // ============================================================
    // Bulk partial + wraparound
    // ============================================================

    inline void test_static_bulk_partial_and_wrap() {
        static_banner("Static SPSCQueue: partial bulk + wraparound");

        volt::SPSCQueueStatic<int, 8> q;

        std::vector<int> first(6);
        std::iota(first.begin(), first.end(), 100);

        VOLT_STATIC_SPSC_CHECK(
            q.push_bulk(std::span<const int>(first)) == 6
        );

        std::vector<int> out1(4, -1);

        VOLT_STATIC_SPSC_CHECK(
            q.pop_bulk(std::span<int>(out1)) == 4
        );

        for (usize i = 0; i < out1.size(); ++i) {
            VOLT_STATIC_SPSC_CHECK(
                out1[i] == static_cast<int>(100 + i)
            );
        }

        std::vector<int> second(6);
        std::iota(second.begin(), second.end(), 200);

        VOLT_STATIC_SPSC_CHECK(
            q.push_bulk(std::span<const int>(second)) == 6
        );

        std::vector<int> out2(8, -1);

        const usize popped =
            q.pop_bulk(std::span<int>(out2));

        VOLT_STATIC_SPSC_CHECK(popped == 8);

        const std::vector<int> expected{
            104, 105,
            200, 201, 202, 203, 204, 205
        };

        VOLT_STATIC_SPSC_CHECK(out2 == expected);
        VOLT_STATIC_SPSC_CHECK(q.empty());

        std::cout << "PASS\n";
    }


    // ============================================================
    // front / back
    // ============================================================

    inline void test_static_front_back() {
        static_banner("Static SPSCQueue: try_front / try_back");

        volt::SPSCQueueStatic<int, 8> q;

        VOLT_STATIC_SPSC_CHECK(q.try_front() == nullptr);
        VOLT_STATIC_SPSC_CHECK(q.try_back() == nullptr);

        VOLT_STATIC_SPSC_CHECK(q.push(10));
        VOLT_STATIC_SPSC_CHECK(q.push(20));
        VOLT_STATIC_SPSC_CHECK(q.push(30));

        auto* front = q.try_front();
        auto* back = q.try_back();

        VOLT_STATIC_SPSC_CHECK(front != nullptr);
        VOLT_STATIC_SPSC_CHECK(back != nullptr);

        VOLT_STATIC_SPSC_CHECK(*front == 10);
        VOLT_STATIC_SPSC_CHECK(*back == 30);

        std::cout << "PASS\n";
    }


    // ============================================================
    // Clear
    // ============================================================

    inline void test_static_clear() {
        static_banner("Static SPSCQueue: clear");

        volt::SPSCQueueStatic<int, 8> q;

        for (int i = 0; i < 8; ++i) {
            VOLT_STATIC_SPSC_CHECK(q.push(i));
        }

        VOLT_STATIC_SPSC_CHECK(q.full());

        q.clear();

        VOLT_STATIC_SPSC_CHECK(q.empty());
        VOLT_STATIC_SPSC_CHECK(!q.full());
        VOLT_STATIC_SPSC_CHECK(q.size() == 0);

        VOLT_STATIC_SPSC_CHECK(q.push(123));

        int out = 0;

        VOLT_STATIC_SPSC_CHECK(q.pop(out));
        VOLT_STATIC_SPSC_CHECK(out == 123);

        std::cout << "PASS\n";
    }


    // ============================================================
    // Concurrent stress
    // ============================================================

    struct StaticStressResult {
        std::uint64_t produced = 0;
        std::uint64_t consumed = 0;
        std::uint64_t mismatches = 0;
    };

    inline StaticStressResult run_static_stress(usize item_count) {
        volt::SPSCQueueStatic<std::uint64_t, 1024> q;

        std::atomic<std::uint64_t> produced{ 0 };
        std::atomic<std::uint64_t> consumed{ 0 };
        std::atomic<std::uint64_t> mismatches{ 0 };

        std::thread producer([&] {
            for (std::uint64_t i = 0; i < item_count; ++i) {
                while (!q.push(i)) {
                    std::this_thread::yield();
                }

                produced.fetch_add(1, std::memory_order_relaxed);
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
                    mismatches.fetch_add(
                        1,
                        std::memory_order_relaxed
                    );
                }

                ++expected;

                consumed.fetch_add(
                    1,
                    std::memory_order_relaxed
                );
            }
            });

        producer.join();
        consumer.join();

        return {
            produced.load(std::memory_order_relaxed),
            consumed.load(std::memory_order_relaxed),
            mismatches.load(std::memory_order_relaxed)
        };
    }

    inline void test_static_stress() {
        static_banner("Static SPSCQueue: concurrent stress");

        constexpr usize count = 2'000'000;

        const auto result = run_static_stress(count);

        VOLT_STATIC_SPSC_CHECK(result.produced == count);
        VOLT_STATIC_SPSC_CHECK(result.consumed == count);
        VOLT_STATIC_SPSC_CHECK(result.mismatches == 0);

        std::cout << "PASS"
            << " | produced=" << result.produced
            << " consumed=" << result.consumed
            << " mismatches=" << result.mismatches
            << '\n';
    }


    // ============================================================
    // Tiny capacity stress
    // ============================================================

    inline void test_static_tiny_capacity() {
        static_banner("Static SPSCQueue: tiny capacity stress");

        constexpr usize count = 500'000;

        {
            volt::SPSCQueueStatic<std::uint64_t, 4> q;

            std::thread producer([&] {
                for (std::uint64_t i = 0; i < count; ++i) {
                    while (!q.push(i))
                        std::this_thread::yield();
                }
                });

            std::thread consumer([&] {
                std::uint64_t expected = 0;

                while (expected < count) {
                    std::uint64_t value = 0;

                    if (!q.pop(value)) {
                        std::this_thread::yield();
                        continue;
                    }

                    VOLT_STATIC_SPSC_CHECK(value == expected);
                    ++expected;
                }
                });

            producer.join();
            consumer.join();
        }

        std::cout << "PASS\n";
    }


    // ============================================================
    // Suite
    // ============================================================

    inline int run_static_spsc_suite() {
        try {
            static_banner("Volt Static SPSCQueue test suite");

            test_static_capacity_and_boundaries();
            test_static_wraparound();
            test_static_lvalue_rvalue_emplace();
            test_static_move_only();
            test_static_lifetime();
            test_static_bulk();
            test_static_bulk_partial_and_wrap();
            test_static_front_back();
            test_static_clear();
            test_static_stress();
            test_static_tiny_capacity();

            std::cout << "\nALL STATIC SPSC TESTS PASSED\n";
            return 0;
        }
        catch (const StaticTestFailure& e) {
            std::cerr << "\nTEST FAILURE: "
                << e.what() << '\n';
            return 1;
        }
        catch (const std::exception& e) {
            std::cerr << "\nUNEXPECTED EXCEPTION: "
                << e.what() << '\n';
            return 2;
        }
        catch (...) {
            std::cerr << "\nUNKNOWN EXCEPTION\n";
            return 3;
        }
    }

#undef VOLT_STATIC_SPSC_CHECK

} // namespace volt::tests