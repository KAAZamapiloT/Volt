#pragma once

#include <volt/Containers/RingBuffer.hpp>

#include <chrono>
#include <cstddef>
#include <iomanip>
#include <iostream>
#include <string>

namespace volt::tests {

namespace {

template<typename Buffer>
double run_benchmark(Buffer& buffer, std::size_t operations)
{
    std::size_t checksum = 0;

    const auto start = std::chrono::steady_clock::now();

    for (std::size_t i = 0; i < operations; ++i) {

        if (buffer.full()) {
            checksum += buffer.front().size();
            buffer.pop();
        }

        buffer.push(
            std::string("RingBuffer_Test_String_") +
            std::to_string(i)
        );
    }

    while (!buffer.empty()) {
        checksum += buffer.front().size();
        buffer.pop();
    }

    const auto end = std::chrono::steady_clock::now();

    std::cout << "Checksum: " << checksum << '\n';

    return std::chrono::duration<double, std::milli>(
        end - start
    ).count();
}

} // anonymous namespace


void benchmark_buffers()
{
    constexpr std::size_t Capacity = 4096;
    constexpr std::size_t Operations = 10'000'000;
    constexpr std::size_t Runs = 5;

    std::cout << "\n=== Volt Ring Buffer Benchmark ===\n";
    std::cout << "Capacity:   " << Capacity << '\n';
    std::cout << "Operations: " << Operations << '\n';
    std::cout << "Runs:        " << Runs << "\n\n";


    // ============================================================
    // Static RingBuffer
    // T[Capacity]
    // Logical pop
    // ============================================================

    double static_total = 0.0;

    for (std::size_t run = 0; run < Runs; ++run) {

        RingBuffer<std::string, Capacity> buffer;

        const double time =
            run_benchmark(buffer, Operations);

        static_total += time;

        std::cout
            << "Static Run "
            << run + 1
            << ": "
            << std::fixed
            << std::setprecision(3)
            << time
            << " ms\n";
    }

    const double static_average =
        static_total / Runs;


    // ============================================================
    // Dynamic RingBuffer
    // Raw storage
    // Physical destruction on pop
    // ============================================================

    double dynamic_total = 0.0;

    for (std::size_t run = 0; run < Runs; ++run) {

        RingBuffer<std::string, 0> buffer(Capacity);

        const double time =
            run_benchmark(buffer, Operations);

        dynamic_total += time;

        std::cout
            << "Dynamic Run "
            << run + 1
            << ": "
            << std::fixed
            << std::setprecision(3)
            << time
            << " ms\n";
    }

    const double dynamic_average =
        dynamic_total / Runs;


    // ============================================================
    // Results
    // ============================================================

    std::cout << "\n=== Results ===\n";

    std::cout
        << "Static average:  "
        << static_average
        << " ms\n";

    std::cout
        << "Dynamic average: "
        << dynamic_average
        << " ms\n";

    if (static_average < dynamic_average) {

        const double difference =
            dynamic_average - static_average;

        const double percent =
            (difference / dynamic_average) * 100.0;

        std::cout
            << "Static is faster by "
            << difference
            << " ms ("
            << percent
            << "%)\n";
    }
    else {

        const double difference =
            static_average - dynamic_average;

        const double percent =
            (difference / static_average) * 100.0;

        std::cout
            << "Dynamic is faster by "
            << difference
            << " ms ("
            << percent
            << "%)\n";
    }
}

} // namespace volt::tests