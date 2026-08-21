#pragma once

#include <bit>
#include <cassert>
#include <cstddef>
#include <cstdint>
#include <memory>
#include <new>
#include <utility>
#include <list>
#include<volt/types/EngineTypes.hpp>


namespace volt {
	

	/// <summary>
	/// A simple buddy allocators allocates in power of 2 
	/// </summary>
	/// <typeparam name="T"></typeparam>
	template<typename T>
	class BuddyAllocator {

		struct FreeBlock {
			FreeBlock* next;
			FreeBlock* prev;
		};
	public:
		T* allocate(usize blocksize) {
			assert(std::has_single_bit(blocksize));

			T* mem = static_cast<T*>(operator new(sizeof(T) * blocksize, std::align_val_t{ alignof(T) }));
			
			return mem;
		}
		void deallocate(T* ptr) {
			if (!ptr) {
				return;
			}
			std::destroy_at(ptr);
		}


		
	
	private:
		std::list<Block*> allocated_blocks;

		static constexpr usize MinBlockSize = 64;
		static constexpr usize MaxBlockSize = 1024 * 1024;
		static constexpr usize NumOrders =
			std::bit_width(MaxBlockSize / MinBlockSize);
		std::byte* m_memory = nullptr;

		FreeBlock* m_freeLists[NumOrders]{};
	};
}