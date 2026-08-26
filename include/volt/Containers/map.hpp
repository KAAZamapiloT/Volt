#pragma once
#include<bit>
#include<cstddef>
#include<cassert>



namespace volt {

	template<typename K,typename V>
	class map {
		static constexpr bool RED = false;
		static constexpr bool BLACK = true;

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
			root_ = nullptr;
		}

		bool insert(K key,V value) {
			
			DataNode* node = bst_insert(key, value);

			if (!node) {
				return false;
			}
	
			node->color = RED;

		
			insert_fixup(node);

			root_->color = BLACK;

			return true;
		}
		void remove(K key) {

		}
		
		V get_key(K key) {
			DataNode* trav = root_;
			while (trav) {
				parent = trav;
				if (trav->KEY < key) {
					trav = trav->right;
				}
				else if (trav->KEY > key) {
					trav = trav->left;
				}
				else {
					return trav->DATA;
				}

			}

			// returningn a dummy object
			V obj;
			return obj;
		}


		operator [](K key) {
			DataNode* trav = root_;
			while (trav) {
				parent = trav;
				if (trav->KEY < key) {
					trav = trav->right;
				}
				else if (trav->KEY > key) {
					trav = trav->left;
				}
				else {
					return trav->DATA;
				}

			}
			// not present insert it key
			V val;
			insert(key, val);
			return val;
		}
	private:
		DataNode* root_;

		DataNode* bst_insert( K key, V value) {
			DataNode* node = new DataNode();
			node->KEY = key;
			node->DATA = value;
			node->left = nullptr;
			node->right = nullptr;
			node->color = RED;

			DataNode* trav = root_;
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
					return nullptr;
				}
				
			}
			node->parent = parent;
	 
			if (!parent) {
				root_ = node;
			}
			else if (parent->KEY < key) {
				parent->right = node;
			}
			else {
				parent->left = node;
			}
			return node;
		}
		

		void insert_fixup(DataNode* node) {
			if ((node->parent == nullptr)||(node->parent->color != node->color)) {
				return;
			}
			
			// if parent is red 
			while (node != root_ && node->parent->color == RED) {
				if (node->parent == node->parent->parent->left) {
					DataNode* y = node->parent->parent->right;
					if (y&&!y->color) {
						node->parent->color = BLACK;
						y->color = BLACK;
						node->parent->parent->color = false;
						node = node->parent->parent;

					}
					else if (node == node->parent->right) {
						node = node->parent;
						rotate_left(node);
						node->parent->color = BLACK;

						node->parent->parent->color = RED;

						rotate_right(node->parent->parent);

					}
				}
				else {
					DataNode* y = node->parent->parent->left;
					if (y&&!y->color) {
						node->parent->color = BLACK;
						y->color = BLACK;
						node->parent->parent->color = RED;
						node = node->parent->parent;

					}
					else if (node == node->parent->left) {
						node = node->parent;
						rotate_left(node);
						node->parent->color = BLACK;
						node->parent->parent->color = RED;
						rotate_right(node->parent->parent);
					}
				}
			}
			root_->color = BLACK;
		}

		void rotate_left(DataNode* x) {
			DataNode* y = x->right;

			
			x->right = y->left;

			if (y->left) {
				y->left->parent = x;
			}

			y->parent = x->parent;

			if (!x->parent) {
				root_ = y;
			}
			else if (x == x->parent->left) {
				x->parent->left = y;
			}
			else {
				x->parent->right = y;
			}

			y->left = x;
			x->parent = y;
		}
		
		void rotate_right(DataNode* x) {
			DataNode* y = x->left;
			x->left = y->right;

			if (y->right) {
				y->right->parent = x;
			}
	

			y->parent = x->parent;

			if (!x->parent) {
				root_ = y;
			}
			else if (x == x->parent->left) {
				x->parent->left = y;
			}
			else {
				x->parent->right= y;
			}

			y->right = x;
			x->parent = y;
		}
	};

}

