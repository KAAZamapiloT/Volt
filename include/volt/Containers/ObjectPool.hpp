#pragma once
#include<memory>
#include<volt/types/EngineTypes.hpp>
#include<volt/types/uniqueptr.hpp>

namespace volt {


	/// <summary>
	/// CRATES A SIMPLE POOL THAT ALLOCATES AND DEALLOCATES OBJECTS OF TYPE T.
	/// IT IS NOT THREAD SAFE AND DOES NOT HANDLE RESIZING.
	/// </summary>
	/// <typeparam name="T"></typeparam>

	template<typename T>
	class ObjectPool {
	public:
		ObjectPool(usize s):size_(0),capacity_(s){
			for(int i=0;i<capacity_;i++){
				pool_[i] = allocate();
			}
		};

		~ObjectPool() {
		};
		
		T* allocate() {

			if (capacity_ == size_) {
				return nullptr;
			}
			++size_;
			return new T();
		}
		bool deallocate(T* obj) {

			// validation
			if (!present(obj)) {
				return false;
			 }
			if(obj == nullptr) {
				return false;
			}
			--size_;

			delete obj;
			return true;
		}

	private:
		usize size_;
		usize capacity_;
		volt::unique_ptr<T[]> pool_;

		bool present(T* obj) {
			for (int i = 0; i < capacity_; i++) {
				if (pool_[i] == obj) {
					return true;
				}
			}
			return false;
		}

	};
}