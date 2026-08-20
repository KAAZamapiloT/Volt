#pragma once

#include <bit>
#include <cstddef>
#include <cstdint>
#include <cassert>
#include<volt/types/EngineTypes.hpp>

namespace volt {
	/// <summary>
	/// A simple Linear allocator
	/// </summary>
	class LinearAllocator {
	public:
		LinearAllocator(void* memory, usize size) noexcept
			: begin_(static_cast<std::byte*>(memory)),
			current_(begin_),
			end_(begin_) {

			assert(memory != nullptr || size == 0);

			if (memory)
				end_ = begin_ + size;
		}

		void* allocate(usize size, usize alignment = alignof(std::max_align_t)) noexcept {
			assert(alignment > 0);
			assert(std::has_single_bit(alignment));

			const auto current = reinterpret_cast<std::uintptr_t>(current_);

			const auto aligned = (current + alignment - 1) & ~(alignment - 1);

			const auto end = reinterpret_cast<std::uintptr_t>(end_);
			if (aligned < current)
				return nullptr; 

			if (aligned > end)
				return nullptr;

			if (size > end - aligned)
				return nullptr;

			const auto next = aligned + size;

			current_ = reinterpret_cast<std::byte*>(next);

			return reinterpret_cast<void*>(aligned);
		}


		void reset() noexcept {
			current_ = begin_;
		}
		usize used() const noexcept {
			return static_cast<usize>(current_ - begin_);

		}
		usize capacity() const noexcept {
			return static_cast<usize>(end_ - begin_);
		}

		usize remaining() const noexcept {
			return static_cast<usize>(end_ - current_);
		}
		Marker mark() const noexcept {
			return current_;
		}
		void rewind(Marker marker) noexcept {
		
			assert(marker >= begin_);
			assert(marker <= current_);

			current_ = marker;
		}
	private:
		std::byte* begin_;
		std::byte* current_;
		std::byte* end_;


	};

}