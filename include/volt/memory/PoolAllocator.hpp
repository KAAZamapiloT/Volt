#pragma once

#include <algorithm>
#include <cassert>
#include <cstddef>
#include <cstdint>
#include <limits>
#include <memory>
#include <utility>
#include<volt/types/EngineTypes.hpp>


/// <summary>
/// I TOOK INSPIRATION  FROM A PAPER I FOUND NAMED
/// Fast Efficient Fixed-Size Memory Pool: No Loops and No Overhead
/// LINK https://www.alphaxiv.org/pdf/2210.16471v1
/// </summary>

namespace volt {
	
	/// <summary>
	/// A simple pool allocator
	/// </summary>
	/// <typeparam name="T"></typeparam>
	/// <typeparam name="Capacity"></typeparam>
	template<typename T,usize Capacity>
	class PoolAllocator {
		using Index = std::uint32_t;
		static constexpr usize BitCount =
			(Capacity + 63) / 64;
#ifdef VOLT_DEBUG
		std::uint64_t allocation_bits_[BitCount]{};
#endif

#ifdef VOLT_DEBUG

		bool is_allocated(Index index) const noexcept {
			const usize word = index / 64;
			const usize bit = index % 64;

			return (allocation_bits_[word] &
				(std::uint64_t{ 1 } << bit)) != 0;
		}

		void mark_allocated(Index index) noexcept {
			const usize word = index / 64;
			const usize bit = index % 64;

			allocation_bits_[word] |=
				(std::uint64_t{ 1 } << bit);
		}

		void mark_free(Index index) noexcept {
			const usize word = index / 64;
			const usize bit = index % 64;

			allocation_bits_[word] &=
				~(std::uint64_t{ 1 } << bit);
		}

#endif
		

		static constexpr Index InvalidIndex =
			std::numeric_limits<Index>::max();

		static constexpr usize RawSlotSize =
			std::max(sizeof(T), sizeof(Index));

		static constexpr usize SlotAlignment =
			std::max(alignof(T), alignof(Index));

		static constexpr usize SlotSize =
			(RawSlotSize + SlotAlignment - 1)
			& ~(SlotAlignment - 1);

		static_assert(Capacity > 0);
		static_assert(
			Capacity <= static_cast<usize>(InvalidIndex),
			"Pool capacity exceeds Index range"
			);

		static_assert(std::has_single_bit(SlotAlignment));
			
		static_assert(
			SlotSize% SlotAlignment == 0
			);
	public:
		PoolAllocator() noexcept {
			
		}

		~PoolAllocator() noexcept {
			assert(size_ == 0);
		}

		PoolAllocator(const PoolAllocator&) = delete;
		PoolAllocator& operator=(const PoolAllocator&) = delete;

		PoolAllocator(PoolAllocator&&) = delete;
		PoolAllocator& operator=(PoolAllocator&&) = delete;
		
		[[nodiscard]]
		T* allocate() noexcept {
			if (free_count_==0) {
				return nullptr;
			}
			if (num_initialized_ < Capacity) {
				const Index index = static_cast<Index>(num_initialized_);
				const Index next = (num_initialized_ + 1) < Capacity ? static_cast<Index>(num_initialized_ + 1) : InvalidIndex;
				
				write_next(index, next);
				++num_initialized_;
			}
			const Index index = free_head_;
			free_head_ = read_next(index);

#ifdef VOLT_DEBUG
			assert(!is_allocated(index));
			mark_allocated(index);
#endif

			--free_count_;
			++size_;
			return slot_as_t(index);
		}

		void deallocate(T*ptr)noexcept {
			if (!ptr) {
				return;
			}
			assert(owns(ptr));

			const Index index = index_from_ptr(ptr);
		
#ifdef VOLT_DEBUG
			assert(
				is_allocated(index) &&
				"PoolAllocator: double free or invalid deallocation"
			);

			mark_free(index);
#endif

			write_next(index, free_head_);
			free_head_ = index;


			--size_;
			++free_count_;
		}



			template<class... Args>
			T* create(Args&&... args) {
				T* ptr = allocate();

				if (!ptr) {
					return nullptr;
				}
				try {
					return std::construct_at(
						ptr,
						std::forward<Args>(args)...
					);
				
				}
				catch (...) {
					deallocate(ptr);
					throw;
				}
			
			}

			void destroy(T*ptr) noexcept {

				if (!ptr) {
					return;
				}

				assert(owns(ptr));


				std::destroy_at(ptr);
				deallocate(ptr);


			}


		[[nodiscard]]
		usize size() const noexcept {
			return size_;
		}

		[[nodiscard]]
		usize capacity() const noexcept {
			return Capacity;
		}

		[[nodiscard]]
		usize available() const noexcept {
			return free_count_;
		}

		[[nodiscard]]
		bool empty() const noexcept {
			return size_ == 0;
		}

		[[nodiscard]]
		bool full() const noexcept {
			return free_count_ == 0;
		}
	private:
		Index free_head_{ 0 };
		Index num_initialized_{ 0 };
		usize free_count_{ Capacity };

		alignas(SlotAlignment)
			std::byte storage_[SlotSize * Capacity];

		usize size_{ 0 };

		[[nodiscard]]
		std::byte* slot_address(Index index) noexcept {
			return storage_ +
				static_cast<usize>(index) * SlotSize;
		}

		[[nodiscard]]
		const std::byte*
			slot_address(Index index) const noexcept {
			return storage_ +
				static_cast<usize>(index) * SlotSize;
		}

		[[nodiscard]]
		T* slot_as_t(Index index) noexcept {
			return std::launder(
				reinterpret_cast<T*>(slot_address(index))
			);
		}

		[[nodiscard]]
		Index read_next(Index index) const noexcept {
			return *reinterpret_cast<const Index*>(
				slot_address(index)
				);
		}

		void write_next(
			Index index,
			Index next
		) noexcept {
			*reinterpret_cast<Index*>(
				slot_address(index)
				) = next;
		}

		[[nodiscard]]
		usize index_from_ptr(const T* ptr) const noexcept {
			const auto begin =
				reinterpret_cast<std::uintptr_t>(storage_);

			const auto address =
				reinterpret_cast<std::uintptr_t>(ptr);

			return static_cast<usize>(
				(address - begin) / SlotSize
				);
		}

		[[nodiscard]]
		bool owns(const T* ptr) const noexcept {
			if (!ptr)
				return false;

			const auto begin =
				reinterpret_cast<std::uintptr_t>(storage_);

			const auto end =
				begin + SlotSize * Capacity;

			const auto address =
				reinterpret_cast<std::uintptr_t>(ptr);

			if (address < begin || address >= end)
				return false;

			return ((address - begin) % SlotSize) == 0;
		}
};
	


}