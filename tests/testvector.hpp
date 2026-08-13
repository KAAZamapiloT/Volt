#pragma once

#include <algorithm>
#include <chrono>
#include <cstddef>
#include <cstdint>
#include <iomanip>
#include <iostream>
#include <memory>
#include <numeric>
#include <string>
#include <string_view>
#include <utility>
#include <vector>
#include<volt/Containers/SmallVector.hpp>
namespace volt::tests {

using Clock = std::chrono::steady_clock;
using Nanoseconds = std::chrono::nanoseconds;

struct BenchmarkResult {
    std::string name;
    double average_ms = 0.0;
    double min_ms = 0.0;
    double max_ms = 0.0;
    std::uint64_t checksum = 0;
};

inline volatile std::uint64_t g_sink = 0;

template <typename Fn>
BenchmarkResult benchmark(std::string_view name,
                          Fn&& fn,
                          std::size_t warmup_runs = 2,
                          std::size_t measured_runs = 8) {
    for (std::size_t i = 0; i < warmup_runs; ++i) {
        const auto checksum = fn();
        g_sink ^= checksum;
    }

    std::vector<double> times_ms;
    times_ms.reserve(measured_runs);

    std::uint64_t final_checksum = 0;

    for (std::size_t i = 0; i < measured_runs; ++i) {
        const auto start = Clock::now();
        const auto checksum = fn();
        const auto end = Clock::now();

        const auto ns =
            std::chrono::duration_cast<Nanoseconds>(end - start).count();

        times_ms.push_back(static_cast<double>(ns) / 1'000'000.0);
        final_checksum ^= checksum;
        g_sink ^= checksum;
    }

    const auto [min_it, max_it] =
        std::minmax_element(times_ms.begin(), times_ms.end());

    const double sum =
        std::accumulate(times_ms.begin(), times_ms.end(), 0.0);

    return BenchmarkResult{
        std::string{name},
        sum / static_cast<double>(times_ms.size()),
        *min_it,
        *max_it,
        final_checksum
    };
}

inline void print_result(const BenchmarkResult& r) {
    std::cout << std::left
              << std::setw(34) << r.name
              << " | avg " << std::setw(10) << std::fixed
              << std::setprecision(3) << r.average_ms
              << " ms | min " << std::setw(10) << r.min_ms
              << " ms | max " << std::setw(10) << r.max_ms
              << " ms | checksum " << r.checksum
              << '\n';
}

inline void print_header(std::string_view title) {
    std::cout << "\n========================================\n"
              << " " << title << '\n'
              << "========================================\n";
}

template <typename T>
std::uint64_t checksum_range(const T* data, std::size_t count) {
    std::uint64_t checksum = 0;
    for (std::size_t i = 0; i < count; ++i) {
        checksum = checksum * 1315423911ull +
                   static_cast<std::uint64_t>(data[i]);
    }
    return checksum;
}

// ------------------------------------------------------------
// Benchmark 1: push_back into inline capacity.
// This should heavily favor SmallVector when N is large enough
// to hold the complete workload without spilling to the heap.
// ------------------------------------------------------------
template <std::size_t N>
BenchmarkResult benchmark_inline_push(std::size_t count) {
    return benchmark(
        "Volt SmallVector inline push",
        [count]() -> std::uint64_t {
            volt::SmallVector<int, N> values;

            for (std::size_t i = 0; i < count; ++i) {
                values.push_back(static_cast<int>(i));
            }

            std::uint64_t checksum = 0;
            for (std::size_t i = 0; i < values.size(); ++i) {
                checksum += static_cast<std::uint64_t>(values[i]);
            }

            return checksum;
        });
}

// ------------------------------------------------------------
// Benchmark 2: std::vector baseline.
// reserve() is deliberately used here because the comparison
// is against a no-growth dynamic vector.
// ------------------------------------------------------------
inline BenchmarkResult benchmark_std_reserved_push(std::size_t count) {
    return benchmark(
        "std::vector reserved push",
        [count]() -> std::uint64_t {
            std::vector<int> values;
            values.reserve(count);

            for (std::size_t i = 0; i < count; ++i) {
                values.push_back(static_cast<int>(i));
            }

            std::uint64_t checksum = 0;
            for (const int value : values) {
                checksum += static_cast<std::uint64_t>(value);
            }

            return checksum;
        });
}

// ------------------------------------------------------------
// Benchmark 3: growth-heavy benchmark.
//
// SmallVector starts with a tiny inline buffer, then repeatedly
// reallocates. This tests the expensive path you're implementing.
// ------------------------------------------------------------
template <std::size_t N>
BenchmarkResult benchmark_growth_push(std::size_t count) {
    return benchmark(
        "Volt SmallVector growth push",
        [count]() -> std::uint64_t {
            volt::SmallVector<int, N> values;

            for (std::size_t i = 0; i < count; ++i) {
                values.push_back(static_cast<int>(i));
            }

            std::uint64_t checksum = 0;
            for (std::size_t i = 0; i < values.size(); ++i) {
                checksum += static_cast<std::uint64_t>(values[i]);
            }

            return checksum;
        });
}

inline BenchmarkResult benchmark_std_growth_push(std::size_t count) {
    return benchmark(
        "std::vector growth push",
        [count]() -> std::uint64_t {
            std::vector<int> values;

            for (std::size_t i = 0; i < count; ++i) {
                values.push_back(static_cast<int>(i));
            }

            std::uint64_t checksum = 0;
            for (const int value : values) {
                checksum += static_cast<std::uint64_t>(value);
            }

            return checksum;
        });
}

// ------------------------------------------------------------
// Benchmark 4: sequential read.
// Once populated, both structures should have contiguous data.
// This is mostly a sanity check for memory locality.
// ------------------------------------------------------------
template <std::size_t N>
BenchmarkResult benchmark_smallvector_read(std::size_t count) {
    return benchmark(
        "Volt SmallVector sequential read",
        [count]() -> std::uint64_t {
            volt::SmallVector<int, N> values;
            values.reserve(count);

            for (std::size_t i = 0; i < count; ++i) {
                values.push_back(static_cast<int>(i + 1));
            }

            std::uint64_t checksum = 0;
            for (std::size_t i = 0; i < values.size(); ++i) {
                checksum += static_cast<std::uint64_t>(values[i]);
            }

            return checksum;
        });
}

inline BenchmarkResult benchmark_std_read(std::size_t count) {
    return benchmark(
        "std::vector sequential read",
        [count]() -> std::uint64_t {
            std::vector<int> values;
            values.reserve(count);

            for (std::size_t i = 0; i < count; ++i) {
                values.push_back(static_cast<int>(i + 1));
            }

            std::uint64_t checksum = 0;
            for (const int value : values) {
                checksum += static_cast<std::uint64_t>(value);
            }

            return checksum;
        });
}

// ------------------------------------------------------------
// Benchmark 5: emplace_back.
// Uses a non-trivial object so that this exercises construction
// rather than only assignment of integers.
// ------------------------------------------------------------
struct TestObject {
    std::uint64_t id;
    std::uint64_t payload;

    TestObject(std::uint64_t id_, std::uint64_t payload_)
        : id(id_), payload(payload_) {}

    TestObject(const TestObject&) = delete;
    TestObject& operator=(const TestObject&) = delete;

    TestObject(TestObject&&) noexcept = default;
    TestObject& operator=(TestObject&&) noexcept = default;
};

template <std::size_t N>
BenchmarkResult benchmark_smallvector_emplace(std::size_t count) {
    return benchmark(
        "Volt SmallVector emplace_back",
        [count]() -> std::uint64_t {
            volt::SmallVector<TestObject, N> values;

            for (std::size_t i = 0; i < count; ++i) {
                values.emplace_back(
                    static_cast<std::uint64_t>(i),
                    static_cast<std::uint64_t>(i * 3 + 7)
                );
            }

            std::uint64_t checksum = 0;
            for (std::size_t i = 0; i < values.size(); ++i) {
                checksum += values[i].id;
                checksum += values[i].payload;
            }

            return checksum;
        });
}

// ------------------------------------------------------------
// Basic correctness tests.
// These are intentionally simple and should be run before
// trusting benchmark numbers.
// ------------------------------------------------------------
template <std::size_t N>
bool basic_correctness() {
    volt::SmallVector<int, N> values;

    if (!values.empty()) {
        return false;
    }

    constexpr int count = static_cast<int>(N + 8);

    for (int i = 0; i < count; ++i) {
        values.push_back(i * 10);
    }

    if (values.size() != static_cast<std::size_t>(count)) {
        return false;
    }

    for (int i = 0; i < count; ++i) {
        if (values[static_cast<std::size_t>(i)] != i * 10) {
            return false;
        }
    }

    values.pop_back();

    if (values.size() != static_cast<std::size_t>(count - 1)) {
        return false;
    }

    values.clear();

    return values.empty() && values.size() == 0;
}

template <std::size_t N>
void run_all(std::size_t count = 1'000'000) {
    print_header("Volt::SmallVector benchmark");

    std::cout << "Elements per run : " << count << '\n';
    std::cout << "Measured runs    : 8\n";
    std::cout << "Warmup runs      : 2\n";
    std::cout << "Initial capacity : " << N << "\n\n";

    const bool ok = basic_correctness<N>();

    std::cout << "Correctness      : "
              << (ok ? "PASS" : "FAIL") << "\n\n";

    if (!ok) {
        std::cerr << "Benchmark aborted: correctness test failed.\n";
        return;
    }

    print_result(benchmark_inline_push<N>(count));
    print_result(benchmark_std_reserved_push(count));

    print_result(benchmark_growth_push<N>(count));
    print_result(benchmark_std_growth_push(count));

    print_result(benchmark_smallvector_read<N>(count));
    print_result(benchmark_std_read(count));

    print_result(benchmark_smallvector_emplace<N>(count));

    std::cout << '\n';
    std::cout << "Note: checksum values are used to prevent the compiler from\n"
              << "optimizing away the work. Compare matching checksums first.\n";
}

// Optional standalone entry point.
// Define VOLT_TESTVECTOR_MAIN in exactly one translation unit before
// including this header if you want a quick executable benchmark.
//
// Example:
//   #define VOLT_TESTVECTOR_MAIN
//   #include "testvector.hpp"
//
#ifdef VOLT_TESTVECTOR_MAIN
int main() {
    volt::tests::run_all<64>(1'000'000);
    return 0;
}
#endif

} // namespace volt::tests
