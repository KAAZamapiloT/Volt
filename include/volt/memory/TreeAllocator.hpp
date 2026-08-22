#pragma once
#include <bit>
#include <cstddef>
#include <cstdint>
#include <cassert>
#include<volt/types/EngineTypes.hpp>



namespace volt {

	/// <summary>
	/// A tree shaped memory allocation especially a binary sorted tree
	/// </summary>
	/// <typeparam name="T"></typeparam>
	template<typename T>
	class TreeAllocator {

		//RB-tree
		struct Node {
			
			Node* parent = nullptr;
			Node* left_children = nullptr;
			Node* right_children = nullptr;
			usize size = 0;

			// false =red true = black
			bool color;
		};
	public:

		TreeAllocator() {
			// just an initailization
			
			head_ = new Node();

			//RB tree 
			head_->color = true;
		};
		T* allocate(usize data) {

			auto* node = allocate_node(data);

			auto* trav = head_;
               
			//searching a insertion point
			Node* parent = nullptr;
			while (trav) {
				parent = trav;
				if (node->size=trav->size) {
					trav = trav->left_children;
				}
				else{
					trav = trav->right_children;
				}
			}
			node->parent = trav;

			if (!parent) {
				head_ = node;
			}
			else if (parent->size > node->size) {
				parent->left_children = node;
			}
			else {
				parent->right_children = node;
			}

			return reinterpret_cast<T*>(node);
		}

		T* deallocate(T* ptr) {
			if (!ptr) {
				return;
			}
			//Now look at root nodes 
		      
			Node* i=head_;
			while (reinterpret_cast<std::uintptr_t>(i) != reinterpret_cast<std::uintptr_t>(ptr)) {
				if (reinterpret_cast<std::uintptr_t>(i) < reinterpret_cast<std::uintptr_t>(ptr)) {
					i=i->left_children
				}
				else(reinterpret_cast<std::uintptr_t>(i)> reinterpret_cast<std::uintptr_t>(ptr)) {
					i=i->right_children
				}
			}
				if (i == ptr) {
					
					if (i->left_children&&i->right_children) {

						if (i->parent->left_children == ptr) {
							
								if (i->left_children->color != i->parent->color) {
									i->parent->left_children = i->left_children;
									
									i->right_children->parent = i->left_children;

								}
								else if (i->right_children->color != i->parent->color) {
									i->parent->left_children = i->right_children;
								}

						}
						else if (i->parent->right_children == ptr) {

							if (i->left_children->color != i->parent->color) {
								i->parent->left_children = i->left_children;

								i->right_children->parent = i->left_children;

							}
							else if (i->right_children->color != i->parent->color) {
								i->parent->left_children = i->right_children;
							}

						}
					}
					else if(!i->left_children && !i->right_children){
						if (i->parent->left_children == ptr) {
							i->parent->left_children = nullptr;
						}
						else if (i->parent->right_children == ptr) {
							i->parent->right_children == nullptr;
						}
						
					}
					else if (!i->left_children && i->right_children) {
						i->right_children->parent = i->parent;
						if (i->parent->left_children == ptr) {
							i->parent->left_children = i->right_children;
						}
						else if (i->parent->right_children == ptr) {
							i->parent->right_children == i->right_children;
						}
					}
					else if (i->left_children && !i->right_children) {
						i->left_children->parent = i->parent;
						if (i->parent->left_children == ptr) {
							i->parent->left_children = i->left_children;
						}
						else if (i->parent->right_children == ptr) {
							i->parent->right_children == i->left_children;
						}
					}


					std::destroy_at(i, std::align_val_t{ alignof(T) });
					return;
				}
			

		}
	private:

		alignas(T) std::byte* memory_;

		
		Node head_;

		


    };
}


