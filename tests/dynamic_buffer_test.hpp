#pragma once

#include <volt/Containers/DynamicRingBuffer.hpp>

#include <algorithm>
#include <chrono>
#include <cstddef>
#include <iomanip>
#include <iostream>
#include <string>
#include <vector>

namespace volt::tests {

    namespace {

        using Clock = std::chrono::steady_clock;

        struct BenchmarkResult {
            double milliseconds = 0.0;
            std::size_t checksum = 0;
        };

        struct HeavyObject {
            std::string payload;

            explicit HeavyObject(std::size_t size = 128)
                : payload(size, 'X')
            {
            }

            HeavyObject(const HeavyObject&) = default;
            HeavyObject(HeavyObject&&) noexcept = default;

            HeavyObject& operator=(const HeavyObject&) = default;
            HeavyObject& operator=(HeavyObject&&) noexcept = default;

            ~HeavyObject() = default;
        };


        // ------------------------------------------------------------
        // Generic benchmark
        // ------------------------------------------------------------

        template<typename T, typename Producer>
        BenchmarkResult benchmark_growth(
            std::size_t operations,
            Producer&& producer)
        {
            DynamicRingBuffer<T> buffer(8);

            std::size_t checksum = 0;

            const auto start = Clock::now();

            for (std::size_t i = 0; i < operations; ++i) {

                buffer.push(producer(i));

                // Occasionally consume an element so this tests
                // actual ring semantics rather than just growth.
                if (buffer.size() > buffer.capacity() / 2) {
                    checksum += buffer.size();
                    buffer.pop();
                }
            }

            while (!buffer.empty()) {
                checksum += buffer.size();
                buffer.pop();
            }

            const auto end = Clock::now();

            return {
                std::chrono::duration<double, std::milli>(
                    end - start
                ).count(),
                checksum
            };
        }


        // ------------------------------------------------------------
        // Pure growth benchmark
        // ------------------------------------------------------------

        template<typename T, typename Producer>
        BenchmarkResult benchmark_pure_growth(
            std::size_t operations,
            Producer&& producer)
        {
            DynamicRingBuffer<T> buffer(8);

            std::size_t checksum = 0;

            const auto start = Clock::now();

            for (std::size_t i = 0; i < operations; ++i) {
                buffer.push(producer(i));
                checksum += buffer.size();
            }

            const auto end = Clock::now();

            return {
                std::chrono::duration<double, std::milli>(
                    end - start
                ).count(),
                checksum
            };
        }


        // ------------------------------------------------------------
        // Repeated push/pop benchmark
        // ------------------------------------------------------------

        template<typename T, typename Producer, typename Measure>
        BenchmarkResult benchmark_steady_state(
            std::size_t operations,
            std::size_t capacity,
            Producer&& producer,
            Measure&& measure)
        {
            DynamicRingBuffer<T> buffer(capacity);

            std::size_t checksum = 0;

            // Fill initially.
            for (std::size_t i = 0; i < capacity; ++i)
                buffer.push(producer(i));

            const auto start = Clock::now();

            for (std::size_t i = 0; i < operations; ++i) {

                checksum += measure(buffer.front());

                buffer.pop();

                buffer.push(producer(i + capacity));
            }

            const auto end = Clock::now();

            while (!buffer.empty())
                buffer.pop();

            return {
                std::chrono::duration<double, std::milli>(
                    end - start
                ).count(),
                checksum
            };
        }

      
        void print_result(
            const std::string& name,
            const BenchmarkResult& result)
        {
            std::cout
                << std::left
                << std::setw(32)
                << name
                << " : "
                << std::fixed
                << std::setprecision(3)
                << result.milliseconds
                << " ms"
                << " | checksum = "
                << result.checksum
                << '\n';
        }

    } // anonymous namespace


    inline void benchmark_dynamic_ring_buffer()
    {
        constexpr std::size_t Operations = 1'000'000;
        constexpr std::size_t Runs = 5;

        std::cout << '\n';
        std::cout << "========================================\n";
        std::cout << " DynamicRingBuffer Benchmark\n";
        std::cout << "========================================\n";

        std::cout << "Operations : " << Operations << '\n';
        std::cout << "Runs       : " << Runs << "\n\n";


        // ============================================================
        // 1. int - pure growth
        // ============================================================

        {
            double total = 0.0;

            std::cout << "[1] int - pure growth\n";

            for (std::size_t run = 0; run < Runs; ++run) {

                auto result =
                    benchmark_pure_growth<int>(
                        Operations,
                        [](std::size_t i) {
                            return static_cast<int>(i);
                        }
                    );

                total += result.milliseconds;

                print_result(
                    "run " + std::to_string(run + 1),
                    result
                );
            }

            std::cout
                << "average                      : "
                << total / Runs
                << " ms\n\n";
        }


        // ============================================================
        // 2. int - steady state
        // ============================================================

        {
            double total = 0.0;

            std::cout << "[2] int - steady state\n";

            for (std::size_t run = 0; run < Runs; ++run) {

                auto result =
                    benchmark_steady_state<int>(
                        Operations,
                        4096,
                        [](std::size_t i) {
                            return static_cast<int>(i);
                        },
                        [](const int& value) {
                            return static_cast<std::size_t>(value);
                        }
                    );

                total += result.milliseconds;

                print_result(
                    "run " + std::to_string(run + 1),
                    result
                );
            }

            std::cout
                << "average                      : "
                << total / Runs
                << " ms\n\n";
        }


        // ============================================================
        // 3. Small strings / SSO
        // ============================================================

        {
            double total = 0.0;

            std::cout << "[3] std::string - small / SSO\n";

            for (std::size_t run = 0; run < Runs; ++run) {

                auto result =
                    benchmark_pure_growth<std::string>(
                        Operations,
                        [](std::size_t i) {
                            return std::string(
                                "object_" + std::to_string(i)
                            );
                        }
                    );

                total += result.milliseconds;

                print_result(
                    "run " + std::to_string(run + 1),
                    result
                );
            }

            std::cout
                << "average                      : "
                << total / Runs
                << " ms\n\n";
        }


        // ============================================================
        // 4. Large strings
        // ============================================================

        {
            double total = 0.0;

            std::cout << "[4] std::string - large\n";

            for (std::size_t run = 0; run < Runs; ++run) {

                auto result =
                    benchmark_pure_growth<std::string>(
                        Operations / 10,
                        [](std::size_t i) {
                            std::string s(1024, 'X');
                            s[0] = static_cast<char>(i);
                            return s;
                        }
                    );

                total += result.milliseconds;

                print_result(
                    "run " + std::to_string(run + 1),
                    result
                );
            }

            std::cout
                << "average                      : "
                << total / Runs
                << " ms\n\n";
        }


        // ============================================================
        // 5. Heavy RAII object
        // ============================================================

        {
            double total = 0.0;

            std::cout << "[5] HeavyObject - steady state\n";

            for (std::size_t run = 0; run < Runs; ++run) {

                auto result =
                    benchmark_steady_state<HeavyObject>(
                        Operations / 10,
                        4096,
                        [](std::size_t) {
                            return HeavyObject{ 256 };
                        },
                        [](const HeavyObject& value) {
                            return value.payload.size();
                        }
                    );

                total += result.milliseconds;

                print_result(
                    "run " + std::to_string(run + 1),
                    result
                );
            }

            std::cout
                << "average                      : "
                << total / Runs
                << " ms\n\n";
        }


        // ============================================================
        // 6. Bursty workload
        // ============================================================

        {
            std::cout << "[6] Bursty growth\n";

            DynamicRingBuffer<int> buffer(8);

            std::size_t checksum = 0;

            const auto start = Clock::now();

            for (std::size_t burst = 0; burst < 1000; ++burst) {

                const std::size_t count =
                    100 + (burst % 500);

                for (std::size_t i = 0; i < count; ++i)
                    buffer.push(static_cast<int>(i));

                while (buffer.size() > count / 2) {
                    checksum += buffer.front();
                    buffer.pop();
                }
            }

            const auto end = Clock::now();

            const double ms =
                std::chrono::duration<double, std::milli>(
                    end - start
                ).count();

            std::cout
                << "bursty workload              : "
                << ms
                << " ms"
                << " | checksum = "
                << checksum
                << "\n\n";
        }


        // ============================================================
        // Final state test
        // ============================================================

        {
            DynamicRingBuffer<int> buffer(2);

            for (int i = 0; i < 100; ++i)
                buffer.push(i);

            std::cout << "[7] Final correctness check\n";
            std::cout << "size     : " << buffer.size() << '\n';
            std::cout << "capacity : " << buffer.capacity() << '\n';
            std::cout << "front    : " << buffer.front() << '\n';
            std::cout << "back     : " << buffer.back() << '\n';

            while (!buffer.empty())
                buffer.pop();

            std::cout << "empty    : "
                << std::boolalpha
                << buffer.empty()
                << '\n';
        }
    }

} // namespace volt::tests