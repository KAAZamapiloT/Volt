#pragma once
#include<bit>
#include<cstddef>
#include<cassert>



namespace volt {

	template<typename K,typename V>
	class map {
	
		struct DataNode {
			K KEY;
			V DATA;
			DataNode* parent;
			DataNode* left;
			DataNode* right;

			// true =  black 
			//false= red
			bool color;
		};
	
	public:
		map() {

		}

		void insert(K key,V value) {
			
			DataNode* node = bst_insert(key, value);

	
			node->color = RED;

		
			insert_fixup(node);

			root_->color = BLACK;


		}
		void remove(K key) {

		}
		V get_key(K key) {

		}



	private:
		DataNode* root_;

		DataNode* bst_insert( K key, V value) {
			DataNode* val = new DataNode();
			val->KEY = key;
			val->DATA = value;
			DataNode* trav = root_{nullptr};
			DataNode* parent = nullptr;
			while (trav) {
				parent = trav;
				if (trav->KEY < key) {
					trav = trav->right;
				}
				else if(trav->KEY>key){
					trav = trav->left;
				}
				else {
					// replace the current node in map
					node->parent = trav->parent;
					if (trav->parent->left == trav) {
						trav->parent->left = node;
					}
					else if (trav->parent->right == trav) {
						trav->parent->right = node;
					}

					if (trav->left) {
						trav->left->parent = node;
					}
					if (trav->right) {
						trav->right->parent = node;
					}
					node->left = trav->left;
					node->right = trav->right;

					return node;
				}
				
			}
			val->parent = parent;
	 
			if (!parent) {
				root_ = node;
			}
			else if (parent->KEY < key) {
				parent->right = node;
			}
			else {
				parent->left = node;
			}
			return val;
		}
		

		void insert_fixup(DataNode* node) {
			if (node->parent->color != node->color) {
				return;
			}
			if (node->parent == nullptr) {
				return;
			}
			
			
		}
	};

}

