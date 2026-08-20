#pragma once

#include <array>
#include <cassert>
#include <cstddef>
#include <cstdint>
#include <iostream>
#include <new>
#include <stdexcept>
#include <string>
#include <type_traits>
#include <utility>

#include <volt/memory/LinearAllocator.hpp>

namespace volt::tests {

using usize = std::size_t;

struct LinearAllocatorTestFailure : std::runtime_error {
    using std::runtime_error::runtime_error;
};

#define VOLT_LINEAR_CHECK(expr)                                              \
    do {                                                                     \
        if (!(expr)) {                                                       \
            throw ::volt::tests::LinearAllocatorTestFailure(                 \
                std::string("CHECK failed: ") + #expr +                     \
                " at " + __FILE__ + ":" + std::to_string(__LINE__));        \
        }                                                                    \
    } while (false)

inline void linear_banner(const char* title) {
    std::cout
        << "\n============================================================\n"
        << title << '\n'
        << "============================================================\n";
}

// ------------------------------------------------------------
// Basic allocation / usage accounting
// ------------------------------------------------------------

inline void test_linear_basic_allocation() {
    linear_banner("LinearAllocator: basic allocation");

    std::array<std::byte, 1024> storage{};
    volt::LinearAllocator arena(storage.data(), storage.size());

    VOLT_LINEAR_CHECK(arena.capacity() == 1024);
    VOLT_LINEAR_CHECK(arena.used() == 0);
    VOLT_LINEAR_CHECK(arena.remaining() == 1024);

    void* a = arena.allocate(64, 8);
    VOLT_LINEAR_CHECK(a != nullptr);

    VOLT_LINEAR_CHECK(
        reinterpret_cast<std::uintptr_t>(a) % 8 == 0
    );

    VOLT_LINEAR_CHECK(arena.used() == 64);
    VOLT_LINEAR_CHECK(arena.remaining() == 960);

    void* b = arena.allocate(128, 16);
    VOLT_LINEAR_CHECK(b != nullptr);

    VOLT_LINEAR_CHECK(
        reinterpret_cast<std::uintptr_t>(b) % 16 == 0
    );

    VOLT_LINEAR_CHECK(arena.used() >= 192);

    std::cout << "PASS\n";
}

// ------------------------------------------------------------
// Alignment
// ------------------------------------------------------------

inline void test_linear_alignment() {
    linear_banner("LinearAllocator: alignment");

    alignas(64) std::array<std::byte, 2048> storage{};
    volt::LinearAllocator arena(storage.data(), storage.size());

    constexpr usize alignments[] = {
        1, 2, 4, 8, 16, 32, 64
    };

    for (usize alignment : alignments) {
        void* ptr = arena.allocate(1, alignment);

        VOLT_LINEAR_CHECK(ptr != nullptr);

        const auto address =
            reinterpret_cast<std::uintptr_t>(ptr);

        VOLT_LINEAR_CHECK(address % alignment == 0);
    }

    std::cout << "PASS\n";
}

// ------------------------------------------------------------
// Exhaustion
// ------------------------------------------------------------

inline void test_linear_exhaustion() {
    linear_banner("LinearAllocator: exhaustion");

    std::array<std::byte, 128> storage{};
    volt::LinearAllocator arena(storage.data(), storage.size());

    void* first = arena.allocate(64, 1);
    VOLT_LINEAR_CHECK(first != nullptr);

    void* second = arena.allocate(64, 1);
    VOLT_LINEAR_CHECK(second != nullptr);

    void* third = arena.allocate(1, 1);
    VOLT_LINEAR_CHECK(third == nullptr);

    VOLT_LINEAR_CHECK(arena.remaining() == 0);
    VOLT_LINEAR_CHECK(arena.used() == arena.capacity());

    std::cout << "PASS\n";
}

// ------------------------------------------------------------
// Reset
// ------------------------------------------------------------

inline void test_linear_reset() {
    linear_banner("LinearAllocator: reset");

    std::array<std::byte, 512> storage{};
    volt::LinearAllocator arena(storage.data(), storage.size());

    VOLT_LINEAR_CHECK(arena.allocate(100, 8) != nullptr);
    VOLT_LINEAR_CHECK(arena.allocate(80, 16) != nullptr);
    VOLT_LINEAR_CHECK(arena.used() > 0);

    arena.reset();

    VOLT_LINEAR_CHECK(arena.used() == 0);
    VOLT_LINEAR_CHECK(arena.remaining() == arena.capacity());

    void* ptr = arena.allocate(32, 16);
    VOLT_LINEAR_CHECK(ptr == storage.data());

    std::cout << "PASS\n";
}

// ------------------------------------------------------------
// Marker / rewind
// ------------------------------------------------------------

inline void test_linear_marker_rewind() {
    linear_banner("LinearAllocator: marker / rewind");

    std::array<std::byte, 1024> storage{};
    volt::LinearAllocator arena(storage.data(), storage.size());

    void* first = arena.allocate(64, 8);
    VOLT_LINEAR_CHECK(first != nullptr);

    const auto marker = arena.mark();

    void* second = arena.allocate(128, 16);
    void* third = arena.allocate(256, 32);

    VOLT_LINEAR_CHECK(second != nullptr);
    VOLT_LINEAR_CHECK(third != nullptr);

    const usize used_before_rewind = arena.used();
    VOLT_LINEAR_CHECK(used_before_rewind > 64);

    arena.rewind(marker);

    VOLT_LINEAR_CHECK(arena.used() == 64);

    void* reused = arena.allocate(128, 16);
    VOLT_LINEAR_CHECK(reused != nullptr);

    VOLT_LINEAR_CHECK(
        reinterpret_cast<std::uintptr_t>(reused) % 16 == 0
    );

    std::cout << "PASS\n";
}

// ------------------------------------------------------------
// Nested markers
// ------------------------------------------------------------

inline void test_linear_nested_markers() {
    linear_banner("LinearAllocator: nested markers");

    std::array<std::byte, 1024> storage{};
    volt::LinearAllocator arena(storage.data(), storage.size());

    const auto outer = arena.mark();

    VOLT_LINEAR_CHECK(arena.allocate(64, 8) != nullptr);

    const auto inner = arena.mark();

    VOLT_LINEAR_CHECK(arena.allocate(128, 16) != nullptr);
    VOLT_LINEAR_CHECK(arena.allocate(32, 32) != nullptr);

    arena.rewind(inner);

    VOLT_LINEAR_CHECK(arena.used() == 64);

    VOLT_LINEAR_CHECK(arena.allocate(64, 8) != nullptr);

    arena.rewind(outer);

    VOLT_LINEAR_CHECK(arena.used() == 0);
    VOLT_LINEAR_CHECK(arena.remaining() == arena.capacity());

    std::cout << "PASS\n";
}

// ------------------------------------------------------------
// Zero-size allocation contract
// ------------------------------------------------------------

inline void test_linear_zero_size() {
    linear_banner("LinearAllocator: zero-size allocation");

    std::array<std::byte, 128> storage{};
    volt::LinearAllocator arena(storage.data(), storage.size());

    const usize before = arena.used();

    void* ptr = arena.allocate(0, 8);

    // Current allocator contract: zero-size allocation is valid
    // only if the implementation explicitly chooses to support it.
    //
    // This test currently checks that it does not consume storage.
    VOLT_LINEAR_CHECK(arena.used() == before);

    // Avoid requiring a particular pointer result.
    (void)ptr;

    std::cout << "PASS\n";
}

// ------------------------------------------------------------
// Typed object construction on allocator storage
// ------------------------------------------------------------

struct TrackedObject {
    static inline usize constructions = 0;
    static inline usize destructions = 0;

    int value = 0;

    explicit TrackedObject(int v)
        : value(v) {
        ++constructions;
    }

    TrackedObject(const TrackedObject&) = delete;
    TrackedObject& operator=(const TrackedObject&) = delete;

    TrackedObject(TrackedObject&& other) noexcept
        : value(other.value) {
        other.value = -1;
        ++constructions;
    }

    TrackedObject& operator=(TrackedObject&& other) noexcept {
        value = other.value;
        other.value = -1;
        return *this;
    }

    ~TrackedObject() {
        ++destructions;
    }

    static void reset() {
        constructions = 0;
        destructions = 0;
    }
};

inline void test_linear_object_lifetime() {
    linear_banner("LinearAllocator: object lifetime");

    TrackedObject::reset();

    std::array<std::byte, 512> storage{};
    volt::LinearAllocator arena(storage.data(), storage.size());

    void* memory = arena.allocate(
        sizeof(TrackedObject),
        alignof(TrackedObject)
    );

    VOLT_LINEAR_CHECK(memory != nullptr);

    auto* object =
        std::construct_at(
            static_cast<TrackedObject*>(memory),
            123
        );

    VOLT_LINEAR_CHECK(object != nullptr);
    VOLT_LINEAR_CHECK(object->value == 123);
    VOLT_LINEAR_CHECK(TrackedObject::constructions == 1);
    VOLT_LINEAR_CHECK(TrackedObject::destructions == 0);

    std::destroy_at(object);

    VOLT_LINEAR_CHECK(TrackedObject::destructions == 1);

    arena.reset();

    std::cout << "PASS\n";
}

// ------------------------------------------------------------
// Large alignment
// ------------------------------------------------------------

struct alignas(64) OverAligned {
    std::uint64_t values[8]{};
};

inline void test_linear_overaligned() {
    linear_banner("LinearAllocator: over-aligned allocation");

    alignas(64) std::array<std::byte, 2048> storage{};
    volt::LinearAllocator arena(storage.data(), storage.size());

    void* memory = arena.allocate(
        sizeof(OverAligned),
        alignof(OverAligned)
    );

    VOLT_LINEAR_CHECK(memory != nullptr);

    const auto address =
        reinterpret_cast<std::uintptr_t>(memory);

    VOLT_LINEAR_CHECK(address % alignof(OverAligned) == 0);

    auto* object =
        std::construct_at(
            static_cast<OverAligned*>(memory)
        );

    VOLT_LINEAR_CHECK(object != nullptr);

    std::destroy_at(object);

    std::cout << "PASS\n";
}

// ------------------------------------------------------------
// Independent allocations must not overlap
// ------------------------------------------------------------

inline void test_linear_non_overlapping() {
    linear_banner("LinearAllocator: non-overlapping allocations");

    std::array<std::byte, 512> storage{};
    volt::LinearAllocator arena(storage.data(), storage.size());

    void* a = arena.allocate(32, 8);
    void* b = arena.allocate(64, 8);
    void* c = arena.allocate(16, 8);

    VOLT_LINEAR_CHECK(a != nullptr);
    VOLT_LINEAR_CHECK(b != nullptr);
    VOLT_LINEAR_CHECK(c != nullptr);

    const auto a_begin =
        reinterpret_cast<std::uintptr_t>(a);
    const auto b_begin =
        reinterpret_cast<std::uintptr_t>(b);
    const auto c_begin =
        reinterpret_cast<std::uintptr_t>(c);

    VOLT_LINEAR_CHECK(a_begin + 32 <= b_begin);
    VOLT_LINEAR_CHECK(b_begin + 64 <= c_begin);

    std::cout << "PASS\n";
}

// ------------------------------------------------------------
// Suite
// ------------------------------------------------------------

inline int run_linear_allocator_suite() {
    try {
        linear_banner("Volt LinearAllocator test suite");

        test_linear_basic_allocation();
        test_linear_alignment();
        test_linear_exhaustion();
        test_linear_reset();
        test_linear_marker_rewind();
        test_linear_nested_markers();
        test_linear_zero_size();
        test_linear_object_lifetime();
        test_linear_overaligned();
        test_linear_non_overlapping();

        std::cout << "\nALL LINEAR ALLOCATOR TESTS PASSED\n";
        return 0;
    }
    catch (const LinearAllocatorTestFailure& e) {
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

#undef VOLT_LINEAR_CHECK

} // namespace volt::tests

#ifdef VOLT_LINEAR_ALLOCATOR_TEST_MAIN

int main() {
    return volt::tests::run_linear_allocator_suite();
}

#endif
