#pragma once

#include <bit>
#include <cassert>
#include <cstddef>
#include <memory>
#include <new>
#include <type_traits>
#include <utility>
#include <volt/types/EngineTypes.hpp>

namespace volt {

/// <summary>
/// A zero-overhead, single-threaded circular ring buffer strictly constrained to power-of-two capacities.
/// Uses bitwise masking for wrap-around, monotonic 64-bit sequence counters, and uninitialized storage.
/// </summary>
/// <typeparam name="T">Element type</typeparam>
/// <typeparam name="Cap">Buffer capacity (must be a power of two > 0)</typeparam>
template <typename T, std::size_t Cap>
    requires (Cap > 0 && std::has_single_bit(Cap))
class FastRingBuffer {
public:
    static constexpr std::size_t static_capacity = Cap;
    static constexpr std::size_t Mask = Cap - 1;

    constexpr FastRingBuffer() noexcept = default;

    ~FastRingBuffer() {
        clear();
    }

    FastRingBuffer(const FastRingBuffer&) = delete;
    FastRingBuffer& operator=(const FastRingBuffer&) = delete;

    FastRingBuffer(FastRingBuffer&& other) noexcept 
        requires std::is_move_constructible_v<T>
    {
        while (!other.empty()) {
            push(std::move(other.front()));
            other.pop();
        }
    }

    FastRingBuffer& operator=(FastRingBuffer&& other) noexcept 
        requires std::is_move_constructible_v<T>
    {
        if (this != &other) {
            clear();
            while (!other.empty()) {
                push(std::move(other.front()));
                other.pop();
            }
        }
        return *this;
    }

    // ------------------------------------------------------------------------
    // Capacity & Status (All O(1) inline)
    // ------------------------------------------------------------------------
    [[nodiscard]] constexpr std::size_t capacity() const noexcept {
        return Cap;
    }

    [[nodiscard]] constexpr std::size_t size() const noexcept {
        return tail_ - head_;
    }

    [[nodiscard]] constexpr bool empty() const noexcept {
        return head_ == tail_;
    }

    [[nodiscard]] constexpr bool full() const noexcept {
        return (tail_ - head_) == Cap;
    }

    // ------------------------------------------------------------------------
    // Element Access (O(1), zero branches)
    // ------------------------------------------------------------------------
    [[nodiscard]] T& front() noexcept {
        assert(!empty() && "front() called on empty FastRingBuffer");
        return *ptr(head_ & Mask);
    }

    [[nodiscard]] const T& front() const noexcept {
        assert(!empty() && "front() called on empty FastRingBuffer");
        return *ptr(head_ & Mask);
    }

    [[nodiscard]] T& back() noexcept {
        assert(!empty() && "back() called on empty FastRingBuffer");
        return *ptr((tail_ - 1) & Mask);
    }

    [[nodiscard]] const T& back() const noexcept {
        assert(!empty() && "back() called on empty FastRingBuffer");
        return *ptr((tail_ - 1) & Mask);
    }

    // Random access from front: index 0 is front(), index size()-1 is back()
    [[nodiscard]] T& operator[](std::size_t index) noexcept {
        assert(index < size() && "Index out of bounds in FastRingBuffer");
        return *ptr((head_ + index) & Mask);
    }

    [[nodiscard]] const T& operator[](std::size_t index) const noexcept {
        assert(index < size() && "Index out of bounds in FastRingBuffer");
        return *ptr((head_ + index) & Mask);
    }

    // ------------------------------------------------------------------------
    // Push Operations (Bitmask indexing, no division/modulo, no exceptions)
    // ------------------------------------------------------------------------
    bool push(const T& value) {
        if (full()) return false;
        std::construct_at(ptr(tail_ & Mask), value);
        ++tail_;
        return true;
    }

    bool push(T&& value) {
        if (full()) return false;
        std::construct_at(ptr(tail_ & Mask), std::move(value));
        ++tail_;
        return true;
    }

    template <typename... Args>
    bool emplace(Args&&... args) {
        if (full()) return false;
        std::construct_at(ptr(tail_ & Mask), std::forward<Args>(args)...);
        ++tail_;
        return true;
    }

    // Push with overwrite: If full, evicts the oldest item (advancing head)
    void push_overwrite(const T& value) {
        if (full()) {
            pop();
        }
        std::construct_at(ptr(tail_ & Mask), value);
        ++tail_;
    }

    void push_overwrite(T&& value) {
        if (full()) {
            pop();
        }
        std::construct_at(ptr(tail_ & Mask), std::move(value));
        ++tail_;
    }

    // ------------------------------------------------------------------------
    // Pop Operations (O(1), inline destructor call)
    // ------------------------------------------------------------------------
    bool pop() noexcept {
        if (empty()) return false;
        std::destroy_at(ptr(head_ & Mask));
        ++head_;
        return true;
    }

    bool pop(T& out) noexcept {
        if (empty()) return false;
        out = std::move(*ptr(head_ & Mask));
        std::destroy_at(ptr(head_ & Mask));
        ++head_;
        return true;
    }

    void clear() noexcept {
        while (pop()) {
            // Destroys all elements
        }
    }

private:
    [[nodiscard]] T* ptr(std::size_t slot) noexcept {
        return reinterpret_cast<T*>(&storage_[slot * sizeof(T)]);
    }

    [[nodiscard]] const T* ptr(std::size_t slot) const noexcept {
        return reinterpret_cast<const T*>(&storage_[slot * sizeof(T)]);
    }

    // Uninitialized aligned raw storage: zero overhead for construction/destruction
    alignas(alignof(T)) std::byte storage_[Cap * sizeof(T)];

    std::size_t head_ = 0;
    std::size_t tail_ = 0;
};

} // namespace volt
