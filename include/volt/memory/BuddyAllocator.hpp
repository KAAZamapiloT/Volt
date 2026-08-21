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

	class BuddyAllocator {

		struct FreeBlock {
			FreeBlock* next;
			FreeBlock* prev;
		};
	public:

		BuddyAllocator() {

			m_memory = static_cast<std::byte*>(::operator new(MaxBlockSize), std::align_val_t{ MaxBlockSize });

			auto* block = reinterpret_cast<FreeBlock*>(m_memory);

			block->next = nullptr;
			block->prev = nullptr;

			m_freeLists[NumOrders - 1] = block;
		}

		~BuddyAllocator() {
			if (m_memory) {
				::operator delete(m_memory, std::align_val_t{ MaxBlockSize });
			}
		}


		BuddyAllocator(const BuddyAllocator&) = delete;
		BuddyAllocator& operator=(const BuddyAllocator&) = delete;

		BuddyAllocator(BuddyAllocator&&) = delete;
		BuddyAllocator& operator=(BuddyAllocator&&) = delete;

		void* allocate(usize size)
		{
			const usize target = size_to_order(size);

			if (target >= NumOrders)
				return nullptr;

			usize order = find_order(target);

			if (order == NumOrders)
				return nullptr;

			FreeBlock* block = pop(order);

			while (order > target)
			{
				--order;

				auto* buddy =
					reinterpret_cast<FreeBlock*>(
						reinterpret_cast<std::byte*>(block)
						+ block_size(order)
						);

				push(order, buddy);
			}

			return block;
		}
		void deallocate(void* ptr) {
			if (!ptr) {
				return;
			}
			std::destroy_at(ptr);
		}


		FreeBlock* split_block(
			FreeBlock* block,
			usize& current_order,
			usize target_order) noexcept {

			while (current_order > target_order)
			{
				--current_order;

				const usize half_size = block_size(current_order);

				auto* buddy =
					reinterpret_cast<FreeBlock*>(
						reinterpret_cast<std::byte*>(block) + half_size
						);

				push_free(current_order, buddy);
			}

			return block;
		}
	
	private:

		static constexpr usize MinBlockSize = 64;
		static constexpr usize MaxBlockSize = 1024 * 1024;
		static constexpr usize NumOrders =
			std::bit_width(MaxBlockSize / MinBlockSize);
		std::byte* m_memory = nullptr;

		FreeBlock* m_freeLists[NumOrders]{};

		constexpr usize block_size(usize order) const noexcept
		{
			return MinBlockSize << order;
		}
		usize size_to_order(usize size) const noexcept
		{
			if (size <= MinBlockSize)
				return 0;

			const usize blocks =
				(size + MinBlockSize - 1) / MinBlockSize;

			return std::bit_width(blocks - 1);
		}

		void push_free(usize order, FreeBlock* block) {
			block->prev = nullptr;
			block->next = m_freeLists[order];

			if (m_freeLists[order])
				m_freeLists[order]->prev = block;

			m_freeLists[order] = block;
		}

		void remove_free(usize order, FreeBlock* block) {

			if (block->prev) {
				block->prev->next = block->next;
			}
			else {
				m_freeLists[order] = block->next;

			}

			if (block->next) {
				block->next->prev = block->prev;
			}
			block->next = nullptr;
			block->prev = nullptr;

		}
		usize find_available_order(usize wanted) const noexcept {

			for (int order = wanted; wanted < NumOrders; ++order) {
				if (m_freeLists[order]) {
					return order;
				}
			}

			return NumOrders;
		}
	};
}