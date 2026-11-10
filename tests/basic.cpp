#include <cassert>
#include <cstdio>

#include <volt/memory/BuddyAllocator.hpp>

int main() {
    volt::BuddyAllocator allocator;

    // Allocate two 64-byte blocks.
    void* a = allocator.allocate(64);
    void* b = allocator.allocate(64);

    assert(a != nullptr);
    assert(b != nullptr);
    assert(a != b);

    // Return both blocks. They should merge repeatedly
    // until the complete 1 MiB arena is free again.
    allocator.deallocate(a);
    allocator.deallocate(b);

    // A complete-arena allocation must now succeed.
    void* entireArena = allocator.allocate(1024 * 1024);

    assert(entireArena != nullptr);
    assert(entireArena == a);

    // There should be no space for another nonzero allocation.
    assert(allocator.allocate(64) == nullptr);

    allocator.deallocate(entireArena);

    std::puts("PASS: allocation, deallocation, and coalescing");
}