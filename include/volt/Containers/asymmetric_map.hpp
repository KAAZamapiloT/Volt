#pragma once
#include<bit>
#include<cstddef>
#include<cassert>
#include <utility>
#include<volt/types/EngineTypes.hpp>





namespace volt {
	/// <summary>
	/// A asymetric Map based on blacnde binary tree helpful in storing blokcs of asymetric data
	/// in ordered key format
	/// </summary>
	/// <typeparam name="K"></typeparam>
	template<typename K>
	class asymmetric_map {
		static constexpr bool RED = false;
		static constexpr bool BLACK = true;

		using TypeID = usize;
		struct TypeInfo {
			TypeID id;
			void (*destroy)(void*);
			void (*copy)(void*, const void*);
			void (*move)(void*, void*);

			template<typename T>
			TypeID type_id()
			{
				static const char unique;
				return reinterpret_cast<TypeID>(&unique);
			}
		};
		
		struct DataNode {
			K KEY;
			void* DATA;
			TypeInfo type;
			DataNode* parent;
			DataNode* left;
			DataNode* right;

			bool color;
		};

	public:

		asymmetric_map() {
			root_ = nullptr;
		}
		asymmetric_map(const map& other)
		{
			root_ = clone_tree(other.root_, nullptr);
			size_ = other.size_;
		}
		asymmetric_map& operator=(const asymmetric_map& other)
		{
			if (this == &other)
				return *this;

			destroy_tree(root_);

			root_ = clone_tree(other.root_, nullptr);
			size_ = other.size_;

			return *this;
		}
		asymmetric_map(asymmetric_map&& other) noexcept
			: root_(other.root_),
			size_(other.size_)
		{
			other.root_ = nullptr;
			other.size_ = 0;
		}
		asymmetric_map& operator=(asymmetric_map&& other) noexcept
		{
			if (this == &other)
				return *this;

			destroy_tree(root_);

			root_ = other.root_;
			size_ = other.size_;

			other.root_ = nullptr;
			other.size_ = 0;

			return *this;
		}
		~asymmetric_map()
		{
			clear();
		}

		void swap(map& other) noexcept
		{
			std::swap(root_, other.root_);
			std::swap(size_, other.size_);
		}

		template<typename V>
		bool insert(const K& key, V value) {

			DataNode* node = bst_insert(key, value);

			if (!node) {
				return false;
			}
			insert_fixup(node);

			root_->color = BLACK;
			++size_;

			return true;
		}

		bool remove(const K& key)
		{
			DataNode* z = searchNode(key);

			if (!z)
				return false;

			DataNode* y = z;
			bool original_color = y->color;

			DataNode* x = nullptr;
			DataNode* x_parent = nullptr;

			if (z->left == nullptr)
			{
				x = z->right;
				x_parent = z->parent;

				transplant(z, z->right);

				if (x)
					x_parent = x->parent;
			}
			else if (z->right == nullptr)
			{
				x = z->left;
				x_parent = z->parent;

				transplant(z, z->left);

				if (x)
					x_parent = x->parent;
			}
			else
			{
				y = minimum(z->right);
				original_color = y->color;

				x = y->right;

				if (y->parent == z)
				{
					x_parent = y;

					if (x)
						x->parent = y;
				}
				else
				{
					x_parent = y->parent;

					transplant(y, y->right);

					y->right = z->right;
					y->right->parent = y;
				}

				transplant(z, y);

				y->left = z->left;
				y->left->parent = y;

				y->color = z->color;
			}

			delete z;

			if (original_color == BLACK)
				delete_fixup(x, x_parent);
			--size_;
			return true;
		}

		template<typename V>
		V* find(const K& key)
		{
			DataNode* trav = root_;

			while (trav)
			{
				if (key < trav->KEY)
				{
					trav = trav->left;
				}
				else if (trav->KEY < key)
				{
					trav = trav->right;
				}
				else
				{
					return &trav->DATA;
				}
			}

			return nullptr;
		}
		template<typename V>
		const V* find(const K& key) const
		{
			const DataNode* trav = root_;

			while (trav)
			{
				if (key < trav->KEY)
					trav = trav->left;
				else if (trav->KEY < key)
					trav = trav->right;
				else
					return &trav->DATA;
			}

			return nullptr;
		}
		template<typename V>
		V& operator [](const K& key) {
			DataNode* trav = root_;
			while (trav) {
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
			return nullptr;
		}
		usize size() const
		{
			return size_;
		}

		bool empty() const
		{
			return size_ == 0;
		}
		void clear()
		{
			destroy_tree(root_);
			root_ = nullptr;
			size_ = 0;
		}
	private:
		DataNode* root_;
		usize size_ = 0;


		template<typename V>
		DataNode* bst_insert(K key, V value) {

			DataNode* trav = root_;
			DataNode* parent = nullptr;
			while (trav) {
				parent = trav;
				if (trav->KEY < key) {
					trav = trav->right;
				}
				else if (trav->KEY > key) {
					trav = trav->left;
				}
				else {
					return nullptr;
				}

			}
			DataNode* node = new DataNode();
			node->KEY = key;
			node->DATA = value;
			node->left = nullptr;
			node->right = nullptr;
			node->color = RED;
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
			while (node != root_ && node->parent->color == RED) {
				DataNode* parent = node->parent;
				DataNode* grandparent = node->parent->parent;

				if (parent == grandparent->left) {
					DataNode* uncle = grandparent->right;
					if (uncle && uncle->color == RED) {
						parent->color = BLACK;
						uncle->color = BLACK;
						grandparent->color = RED;

						node = grandparent;
					}
					else {
						if (node == parent->right) {
							node = parent;
							rotate_left(node);
							parent = node->parent;
							grandparent = parent->parent;
						}
						parent->color = BLACK;
						grandparent->color = RED;
						rotate_right(grandparent);
					}
				}
				else {
					DataNode* uncle = grandparent->left;

					if (uncle && uncle->color == RED)
					{
						parent->color = BLACK;
						uncle->color = BLACK;
						grandparent->color = RED;

						node = grandparent;
					}
					else
					{
						if (node == parent->left)
						{
							node = parent;
							rotate_right(node);

							parent = node->parent;
							grandparent = parent->parent;
						}

						parent->color = BLACK;
						grandparent->color = RED;
						rotate_left(grandparent);
					}
				}

			}


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
				x->parent->right = y;
			}

			y->right = x;
			x->parent = y;
		}



		void transplant(DataNode* x, DataNode* y) {

			if (!x->parent) {
				root_ = y;
			}
			else if (x->parent->left == x) {
				x->parent->left = y;
			}
			else {
				x->parent->right = y;
			}

			if (y) {
				y->parent = x->parent;
			}
		}
		DataNode* minimum(DataNode* node) {

			while (node->left) {
				node = node->left;
			}
			return node;
		}


		void delete_fixup(DataNode* x, DataNode* parent) {


			while (x != root_ && color_of(x) == BLACK)
			{
				if (x == parent->left)
				{
					DataNode* sibling = parent->right;


					if (color_of(sibling) == RED) {
						sibling->color = BLACK;
						parent->color = RED;
						rotate_left(parent);
						sibling = parent->right;
					}


					if (color_of(sibling ? sibling->left : nullptr) == BLACK && color_of(sibling ? sibling->right : nullptr) == BLACK) {


						if (sibling) {
							sibling->color = RED;
						}

						x = parent;
						parent = x->parent;

					}
					else
					{
						if (color_of(sibling ? sibling->right : nullptr) == BLACK)
						{
							if (sibling && sibling->left)
								sibling->left->color = BLACK;

							if (sibling)
							{
								sibling->color = RED;
								rotate_right(sibling);
							}

							sibling = parent->right;
						}
						if (sibling)
							sibling->color = parent->color;

						parent->color = BLACK;

						if (sibling && sibling->right)
							sibling->right->color = BLACK;

						rotate_left(parent);

						x = root_;
						parent = nullptr;
					}
				}
				else
				{
					DataNode* sibling = parent->left;

					if (color_of(sibling) == RED)
					{
						sibling->color = BLACK;
						parent->color = RED;

						rotate_right(parent);

						sibling = parent->left;
					}

					if (color_of(sibling ? sibling->left : nullptr) == BLACK &&
						color_of(sibling ? sibling->right : nullptr) == BLACK)
					{
						if (sibling)
							sibling->color = RED;

						x = parent;
						parent = x->parent;
					}
					else
					{

						if (color_of(sibling ? sibling->left : nullptr) == BLACK)
						{
							if (sibling && sibling->right)
								sibling->right->color = BLACK;

							if (sibling)
							{
								sibling->color = RED;
								rotate_left(sibling);
							}

							sibling = parent->left;
						}


						if (sibling)
							sibling->color = parent->color;

						parent->color = BLACK;

						if (sibling && sibling->left)
							sibling->left->color = BLACK;

						rotate_right(parent);

						x = root_;
						parent = nullptr;
					}
				}
			}

			if (x)
				x->color = BLACK;

		}

		bool color_of(DataNode* node) const
		{
			return node ? node->color : BLACK;
		}

		DataNode* searchNode(K key) {
			DataNode* trav = root_;

			while (trav) {
				if (trav->KEY < key) {
					trav = trav->right;
				}
				else if (trav->KEY > key) {
					trav = trav->left;
				}
				else {
					return trav;
				}

			}
			return nullptr;
		}

		void destroy_tree(DataNode* node)
		{
			if (!node)
				return;

			destroy_tree(node->left);
			destroy_tree(node->right);

			delete node;
		}

		DataNode* clone_tree(const DataNode* node,const  DataNode* parent)
		{
			if (!node)
				return nullptr;

			DataNode* copy = new DataNode();

			copy->KEY = node->KEY;
			copy->DATA = node->DATA;
			copy->color = node->color;
			copy->parent = parent;

			copy->left = clone_tree(node->left, copy);
			copy->right = clone_tree(node->right, copy);

			return copy;
		}
	};

}

