#pragma once

#include <cassert>
#include <cstddef>
#include <cstdint>
#include <iostream>
#include <memory>
#include <stdexcept>
#include <string>
#include <utility>
#include <vector>

#include <volt/Containers/FastRingBuffer.hpp>

namespace volt::tests {

struct FastRingTestFailure : std::runtime_error {
    using std::runtime_error::runtime_error;
};

#define VOLT_FAST_RING_CHECK(expr)                                             \
    do {                                                                       \
        if (!(expr)) {                                                         \
            throw ::volt::tests::FastRingTestFailure(                          \
                std::string("CHECK failed: ") + #expr +                        \
                " at " + __FILE__ + ":" + std::to_string(__LINE__));          \
        }                                                                      \
    } while (false)

inline void test_fast_ring_buffer_basic() {
    volt::FastRingBuffer<int, 4> rb;

    VOLT_FAST_RING_CHECK(rb.capacity() == 4);
    VOLT_FAST_RING_CHECK(rb.size() == 0);
    VOLT_FAST_RING_CHECK(rb.empty());
    VOLT_FAST_RING_CHECK(!rb.full());

    VOLT_FAST_RING_CHECK(rb.push(10));
    VOLT_FAST_RING_CHECK(rb.push(20));
    VOLT_FAST_RING_CHECK(rb.push(30));
    VOLT_FAST_RING_CHECK(rb.push(40));

    VOLT_FAST_RING_CHECK(rb.full());
    VOLT_FAST_RING_CHECK(!rb.push(50)); // should fail

    VOLT_FAST_RING_CHECK(rb.front() == 10);
    VOLT_FAST_RING_CHECK(rb.back() == 40);
    VOLT_FAST_RING_CHECK(rb[0] == 10);
    VOLT_FAST_RING_CHECK(rb[1] == 20);
    VOLT_FAST_RING_CHECK(rb[2] == 30);
    VOLT_FAST_RING_CHECK(rb[3] == 40);

    int val = 0;
    VOLT_FAST_RING_CHECK(rb.pop(val) && val == 10);
    VOLT_FAST_RING_CHECK(rb.pop(val) && val == 20);
    VOLT_FAST_RING_CHECK(rb.size() == 2);

    VOLT_FAST_RING_CHECK(rb.push(50));
    VOLT_FAST_RING_CHECK(rb.push(60));
    VOLT_FAST_RING_CHECK(rb.full());

    VOLT_FAST_RING_CHECK(rb.pop(val) && val == 30);
    VOLT_FAST_RING_CHECK(rb.pop(val) && val == 40);
    VOLT_FAST_RING_CHECK(rb.pop(val) && val == 50);
    VOLT_FAST_RING_CHECK(rb.pop(val) && val == 60);
    VOLT_FAST_RING_CHECK(rb.empty());
    VOLT_FAST_RING_CHECK(!rb.pop(val));
}

inline void test_fast_ring_buffer_overwrite() {
    volt::FastRingBuffer<int, 4> rb;

    rb.push(1);
    rb.push(2);
    rb.push(3);
    rb.push(4);

    // Overwrite evicts oldest (1)
    rb.push_overwrite(5);
    VOLT_FAST_RING_CHECK(rb.size() == 4);
    VOLT_FAST_RING_CHECK(rb.front() == 2);
    VOLT_FAST_RING_CHECK(rb.back() == 5);

    rb.push_overwrite(6);
    VOLT_FAST_RING_CHECK(rb.front() == 3);
    VOLT_FAST_RING_CHECK(rb.back() == 6);
}

struct LifetimeTracker {
    static inline int live_instances = 0;
    int id = 0;

    LifetimeTracker(int val) : id(val) { ++live_instances; }
    ~LifetimeTracker() { --live_instances; }
    LifetimeTracker(const LifetimeTracker& other) : id(other.id) { ++live_instances; }
    LifetimeTracker(LifetimeTracker&& other) noexcept : id(other.id) { ++live_instances; }
    LifetimeTracker& operator=(const LifetimeTracker&) = default;
    LifetimeTracker& operator=(LifetimeTracker&&) noexcept = default;
};

inline void test_fast_ring_buffer_lifetime() {
    LifetimeTracker::live_instances = 0;
    {
        volt::FastRingBuffer<LifetimeTracker, 8> rb;
        for (int i = 0; i < 8; ++i) {
            rb.emplace(i);
        }
        VOLT_FAST_RING_CHECK(LifetimeTracker::live_instances == 8);

        rb.pop();
        rb.pop();
        VOLT_FAST_RING_CHECK(LifetimeTracker::live_instances == 6);

        rb.clear();
        VOLT_FAST_RING_CHECK(LifetimeTracker::live_instances == 0);
    }
    VOLT_FAST_RING_CHECK(LifetimeTracker::live_instances == 0);
}

inline int run_fast_ring_buffer_tests() {
    try {
        test_fast_ring_buffer_basic();
        test_fast_ring_buffer_overwrite();
        test_fast_ring_buffer_lifetime();
        std::cout << "ALL FAST RING BUFFER TESTS PASSED\n";
        return 0;
    }
    catch (const std::exception& e) {
        std::cerr << "TEST FAILURE: " << e.what() << '\n';
        return 1;
    }
}

#undef VOLT_FAST_RING_CHECK

} // namespace volt::tests
