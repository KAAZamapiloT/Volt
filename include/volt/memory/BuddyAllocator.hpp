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
#include<array>

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

			m_memory = static_cast<std::byte*>(
				::operator new(
					MaxBlockSize,
					std::align_val_t{ MaxBlockSize }
					)
				);

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

			usize order = find_available_order(target);

			if (order == NumOrders)
				return nullptr;

			FreeBlock* block = pop(order);

			if (block == nullptr) {
			return nullptr;
		     }

			block - split_block(block, order, target);

			const usize offset = static_cast<usize>(
				reinterpret_cast<std::byte*>(block) - m_memory
				);

			const usize first = offset / MinBlockSize;
			const usize blockCount = block_size(target) / MinBlockSize;

			m_allocOrder[first] = static_cast<std::int8_t>(target);

			for (usize i = 1; i < blockCount; ++i)
				m_allocOrder[first + i] = AllocationInterior;

			return block;
			
		}

		void deallocate(void* ptr) noexcept {
			if (ptr == nullptr)
				return;

			const auto address = reinterpret_cast<std::uintptr_t>(ptr);
			const auto base = reinterpret_cast<std::uintptr_t>(m_memory);

			if (address < base || address - base >= MaxBlockSize) {
				assert(false && "Pointer is outside BuddyAllocator arena");
				return;
			}

			const usize offset = static_cast<usize>(address - base);


			if (offset % MinBlockSize != 0) {
				assert(false && "Pointer is not a block boundary");
				return;
			}

			const usize index = offset / MinBlockSize;
			const std::int8_t recordedOrder = m_allocOrder[index];

			if (recordedOrder < 0 ||
				static_cast<usize>(recordedOrder) >= NumOrders) {
				assert(false && "Invalid or already freed allocation");
				return;
			}

			usize order = static_cast<usize>(recordedOrder);
			usize currentOffset = offset;

			const usize blockCount = block_size(order) / MinBlockSize;

			for (usize i = 0; i < blockCount; ++i)
				m_allocOrder[index + i] = NotAllocated;

			while (order < NumOrders - 1) {
				const usize buddyOffset =
					currentOffset ^ block_size(order);

				auto* buddyAddress = m_memory + buddyOffset;

				FreeBlock* buddy = find_free_block(
					order,
					buddyAddress
				);

				if (buddy == nullptr)
					break;

				remove_free(order, buddy);

				if (buddyOffset < currentOffset)
					currentOffset = buddyOffset;

				++order;
			}

			auto* mergedBlock = reinterpret_cast<FreeBlock*>(
				m_memory + currentOffset
				);

			push_free(order, mergedBlock);
		}

		FreeBlock* split_block(
			FreeBlock* block,
			usize& current_order,
			usize target_order) noexcept {

			assert(block != nullptr);
			assert(current_order < NumOrders);
			assert(target_order <= current_order);


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

		static constexpr usize NumMinBlocks =
			MaxBlockSize / MinBlockSize;

		static constexpr std::int8_t NotAllocated = -1;
		static constexpr std::int8_t AllocationInterior = -2;

		std::array<std::int8_t, NumMinBlocks> m_allocOrder{};

		FreeBlock* m_freeLists[NumOrders]{};

		constexpr usize block_size(usize order) const noexcept
		{
			return MinBlockSize << order;
		}
		usize size_to_order(usize size) const noexcept
		{
			if (size <= MinBlockSize)
				return 0;

			if (size > MaxBlockSize) {
				return NumOrders;
			}
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

			for (int order = wanted; order< NumOrders; ++order) {
				if (m_freeLists[order]!=nullptr) {
					return order;
				}
			}

			return NumOrders;
		}

		FreeBlock* pop(usize order) {
			if (order >= NumOrders) {
				return nullptr;
			}
			FreeBlock* block = m_freeLists[order];

			if (block != nullptr) {
				remove_free(order, block);
			}
			return block;
		}

		FreeBlock* find_free_block(
			usize order,
			const void* address
		) noexcept {
			if (order >= NumOrders)
				return nullptr;

			for (FreeBlock* block = m_freeLists[order];
				block != nullptr;
				block = block->next) {
				if (static_cast<const void*>(block) == address)
					return block;
			}

			return nullptr;
		}
	};
}