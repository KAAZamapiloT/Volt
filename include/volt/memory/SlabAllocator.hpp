
#pragma once

#include <bit>
#include <cassert>
#include <cstddef>
#include <cstdint>
#include <memory>
#include <new>
#include <utility>
#include <vector>
#include<volt/types/EngineTypes.hpp>

namespace volt{

	/// <summary>
	/// A array based salb allocator uses bitsets to track allocations
	/// </summary>
	/// <typeparam name="T"></typeparam>
	template<typename T>
	class SlabAllocator {
        static constexpr std::size_t SlabCapacity = 64;
        template<typename T>
        struct Slab {
            T* memory = nullptr;
            std::uint64_t free_mask = ~std::uint64_t{ 0 };
        };
	public:
		
        SlabAllocator() = default;

        ~SlabAllocator() {
            for (auto& slab : slabs_) {

                for (usize i = 0; i < SlabCapacity; ++i) {

                    if ((slab.free_mask & (uint64_t{ 1 } << i)) == 0) {
                        std::destroy_at(slab.memory + i);
                    }
                }

                ::operator delete(
                    slab.memory,
                    std::align_val_t{ alignof(T) }
                    );
            }
        }

        T* allocate() {

            for (auto& slab : slabs_) {
                
                if (slab.free_mask != 0) {
                
                const usize index = std::countr_zero(slab.free_mask);

                slab.free_mask &=
                    ~(std::uint64_t{ 1 } << index);

                return slab.memory + index;
                }
            }
            slabs_.push_back(create_slab());

            Slab& slab = slabs_.back();

            const std::size_t index =
                std::countr_zero(slab.free_mask);
            slab.free_mask &= ~(std::uint64_t{ 1 } << index);

            return slab.memory + index;

        }
        void  deallocate(T* ptr) {
            assert(ptr != nullptr);

            for (auto& slab : slabs_) {
                T* begin = slab.memory;
                T* end = begin + SlabCapacity;

                if (ptr >= begin && ptr < end) {
                
                    const usize index = static_cast<usize>(ptr - begin);

                    assert(
                        (slab.free_mask &
                            (std::uint64_t{ 1 } << index)) == 0
                    );
                    std::destroy_at(ptr);
                    slab.free_mask |= std::uint64_t{ 1 } << index;
                    return;
                
                }
            }


            assert(false &&
                "Pointer does not belong to this allocator");
        }

        template<typename... Args>
        T* create(Args&&... args) {
            T* ptr = allocate();

            return std::construct_at(
                ptr,
                std::forward<Args>(args)...
            );
        }

        void destroy(T* ptr) {
            deallocate(ptr);
        }
	private:
        std::vector<Slab> slabs_;

        static Slab create_slab() {
            Slab slab;

            slab.memory = static_cast<T*>(
                ::operator new(
                    sizeof(T) * SlabCapacity,
                    std::align_val_t{alignof(T)}
                    )
                );

        slab.free_mask = ~uint64_t{ 0 };
        return slab;

        }

	};
}