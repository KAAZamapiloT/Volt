#pragma once

#include <BenchmarkHarness.hpp>
#include <volt/Containers/RingBuffer.hpp>
#include <volt/Containers/FastRingBuffer.hpp>
#include <deque>

namespace volt::bench {

inline void run_ringbuffer_benchmarks() {
    constexpr std::size_t OPS = 500'000;
    constexpr std::size_t CAPACITY = 2048;

    // 1. Baseline: std::deque push_back + pop_front
    BenchmarkSuite::instance().measure(
        "RingBuffer",
        "std::deque push/pop (baseline)",
        "std::deque chunked map; push 500k items, pop in 100-item sliding window, checksum",
        OPS,
        sizeof(int),
        [&]() -> std::uint64_t {
            std::deque<int> dq;
            std::uint64_t checksum = 0;
            for (std::size_t i = 0; i < OPS; ++i) {
                int val = static_cast<int>(i);
                do_not_optimize(val);
                dq.push_back(val);
                if (dq.size() >= 100) {
                    int front_val = dq.front();
                    do_not_optimize(front_val);
                    checksum += front_val;
                    dq.pop_front();
                }
            }
            while (!dq.empty()) {
                int front_val = dq.front();
                do_not_optimize(front_val);
                checksum += front_val;
                dq.pop_front();
            }
            return checksum;
        }
    );

    // 2. Volt General RingBuffer: push + pop (supports arbitrary capacity)
    BenchmarkSuite::instance().measure(
        "RingBuffer",
        "volt::RingBuffer<int, 2048>",
        "Volt general circular array; push 500k items, pop in 100-item sliding window, checksum",
        OPS,
        sizeof(int),
        [&]() -> std::uint64_t {
            volt::RingBuffer<int, CAPACITY> rb;
            std::uint64_t checksum = 0;
            for (std::size_t i = 0; i < OPS; ++i) {
                int val = static_cast<int>(i);
                do_not_optimize(val);
                rb.push(val);
                if (rb.size() >= 100) {
                    int front_val = rb.front();
                    do_not_optimize(front_val);
                    checksum += front_val;
                    rb.pop();
                }
            }
            while (!rb.empty()) {
                int front_val = rb.front();
                do_not_optimize(front_val);
                checksum += front_val;
                rb.pop();
            }
            return checksum;
        }
    );

    // 3. Volt FastRingBuffer: power-of-two bitmask indexing & uninitialized storage
    BenchmarkSuite::instance().measure(
        "RingBuffer",
        "volt::FastRingBuffer<int, 2048>",
        "Volt power-of-2 bitmask ring buffer; monotonic sequence counters, no exceptions",
        OPS,
        sizeof(int),
        [&]() -> std::uint64_t {
            volt::FastRingBuffer<int, CAPACITY> frb;
            std::uint64_t checksum = 0;
            for (std::size_t i = 0; i < OPS; ++i) {
                int val = static_cast<int>(i);
                do_not_optimize(val);
                frb.push(val);
                if (frb.size() >= 100) {
                    int front_val = frb.front();
                    do_not_optimize(front_val);
                    checksum += front_val;
                    frb.pop();
                }
            }
            while (!frb.empty()) {
                int front_val = frb.front();
                do_not_optimize(front_val);
                checksum += front_val;
                frb.pop();
            }
            return checksum;
        }
    );
}

} // namespace volt::bench
