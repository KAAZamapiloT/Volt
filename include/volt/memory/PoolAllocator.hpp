#pragma once

#include<bit>
#include<cassert>
#include<cstddef>
#include<utility>
#include<memeory>
#include<volt/types/EngineTypes.hpp>




namespace volt {
	

	template<typename T,usize Capacity>
	class PoolAllocator {
		static_assert(Capacity>0)
	public:
		PoolAllocator() {

		}

	private:
		struct FreeNode {
			FreeNode* next;
		};


		alignas(T)
			std::byte storage_[sizeof(T) * Capacity];

		FreeNode* node{ nullptr };
		usize size_{ 0 };

	};


}