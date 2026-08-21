#pragma once
#include <bit>
#include <cstddef>
#include <cstdint>
#include <cassert>
#include<volt/types/EngineTypes.hpp>



namespace volt {


	template<typename T>
	class TreeAllocator {

		//RB-tree
		struct Node {
			
			void* parent;
			void* left_children;
			void* right_children;

			bool color;
		};
	public:


		T* allocate(usize data) {

			auto* node = allocate_node(data);
			return reinterpret_cast<T*>(node);
		}
	private:

		alignas(T) std::byte* memory_;

		
		Node head;
		Node tail_;
		void* allocate_node(usize data) {

			head->children = static_cast<T*>(::operator new(sizeof(T) * data, std::align_val_t{ alignof(T) });
			

		}


    };
}


