#pragma once

#include <BenchmarkHarness.hpp>
#include <volt/Containers/SPSCQueueStatic.hpp>
#include <condition_variable>
#include <mutex>
#include <queue>
#include <thread>

#if defined(_WIN32)
#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#include <windows.h>
#endif

namespace volt::bench {

// 32-byte engine audio/input event packet
struct alignas(32) StreamPacket {
    uint64_t sequence;
    float channels[6];
};

template <typename T>
class MutexQueue {
public:
    void push(T val) {
        {
            std::lock_guard lock(mutex_);
            queue_.push(std::move(val));
        }
        cv_.notify_one();
    }

    bool pop(T& val) {
        std::unique_lock lock(mutex_);
        if (queue_.empty()) return false;
        val = std::move(queue_.front());
        queue_.pop();
        return true;
    }

private:
    std::mutex mutex_;
    std::condition_variable cv_;
    std::queue<T> queue_;
};

inline void pin_thread_to_core(std::size_t core_id) {
#if defined(_WIN32)
    HANDLE thread = GetCurrentThread();
    DWORD_PTR mask = static_cast<DWORD_PTR>(1ULL << (core_id % 64));
    SetThreadAffinityMask(thread, mask);
#endif
}

inline void run_spsc_benchmarks() {
    constexpr std::size_t OPS = 250'000;
    constexpr std::size_t QUEUE_CAP = 1024;

    // ------------------------------------------------------------------------
    // 1. Multi-threaded Baseline: Mutex + CV Queue (Producer on Core 0, Consumer on Core 1)
    // ------------------------------------------------------------------------
    BenchmarkSuite::instance().measure(
        "SPSC Queue",
        "Mutex Queue 2-thread pinned (baseline)",
        "Thread 0 produces, Thread 1 consumes with mutex lock/cv notify; full checksum",
        OPS,
        sizeof(StreamPacket),
        [&]() -> std::uint64_t {
            MutexQueue<StreamPacket> q;
            std::atomic<uint64_t> consumer_checksum{0};

            std::thread producer([&]() {
                pin_thread_to_core(0);
                for (uint64_t i = 0; i < OPS; ++i) {
                    StreamPacket pkt{ i, {1.0f, 2.0f, 3.0f, 4.0f, 5.0f, 6.0f} };
                    q.push(pkt);
                }
            });

            std::thread consumer([&]() {
                pin_thread_to_core(1);
                uint64_t received = 0;
                uint64_t sum = 0;
                while (received < OPS) {
                    StreamPacket pkt{};
                    if (q.pop(pkt)) {
                        sum += pkt.sequence;
                        ++received;
                    }
                }
                consumer_checksum.store(sum, std::memory_order_relaxed);
            });

            producer.join();
            consumer.join();
            return consumer_checksum.load(std::memory_order_relaxed);
        }
    );

    // ------------------------------------------------------------------------
    // 2. Multi-threaded: volt::SPSCQueueStatic (Wait-Free Ring Buffer, Core Pinned)
    // ------------------------------------------------------------------------
    BenchmarkSuite::instance().measure(
        "SPSC Queue",
        "volt::SPSCQueueStatic 2-thread pinned",
        "Producer Core 0, Consumer Core 1; lock-free acquire/release; full checksum",
        OPS,
        sizeof(StreamPacket),
        [&]() -> std::uint64_t {
            volt::SPSCQueueStatic<StreamPacket, QUEUE_CAP> q;
            std::atomic<uint64_t> consumer_checksum{0};

            std::thread producer([&]() {
                pin_thread_to_core(0);
                for (uint64_t i = 0; i < OPS; ++i) {
                    StreamPacket pkt{ i, {1.0f, 2.0f, 3.0f, 4.0f, 5.0f, 6.0f} };
                    while (!q.push(pkt)) {
                        // spin wait (lock-free)
                    }
                }
            });

            std::thread consumer([&]() {
                pin_thread_to_core(1);
                uint64_t received = 0;
                uint64_t sum = 0;
                while (received < OPS) {
                    StreamPacket pkt{};
                    if (q.pop(pkt)) {
                        sum += pkt.sequence;
                        ++received;
                    }
                }
                consumer_checksum.store(sum, std::memory_order_relaxed);
            });

            producer.join();
            consumer.join();
            return consumer_checksum.load(std::memory_order_relaxed);
        }
    );
}

} // namespace volt::bench
