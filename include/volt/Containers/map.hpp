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
			bool color;
		};
	
	public:
		map() {

		}

		void insert(K key,V value) {
			//search key
		}
		void remove(K key) {

		}
		V get_key(K key) {

		}



	private:

	};

}

