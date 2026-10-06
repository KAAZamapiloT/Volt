#pragma once
#include<memory>
#include<unordered_set>
#include<set>
#include<stdalign.h>
#include<unordered_map>
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
		explicit ObjectPool(usize s):size_(0),capacity_(s){
			pool_ = static_cast<T*>(::operator new(sizeof(T) * capacity_));
			for(int i=0;i<capacity_;i++){
				free_indices_.push_back(i);
			}
		};

		~ObjectPool() {
			for (auto& [ptr, index] : ocuupied_indicies_) {
				std::destroy_at(ptr);
			}

			::operator delete(pool_);
		}

		
		template<typename... Args>
		[[nodiscard]]  
		T* allocate(Args&&...args) {
			if (free_indices_.empty()) { return nullptr; }
			auto free_index = free_indices_.back();
			free_indices_.pop_back();
			T* ptr = new (pool_ + free_index)
				T(std::forward<Args>(args)...);

			ocuupied_indicies_[ptr] = free_index;
			++size_;

			return ptr;
		}

		bool deallocate(T* obj) {

			// validation
			if (obj == nullptr) {
				return false;
			}
			if (!present(obj)) {
				return false;
			 }
		
			--size_;
			int free_index = ocuupied_indicies_[obj];
			ocuupied_indicies_.erase(obj);
			free_indices_.push_back(free_index);
			std::destroy_at(obj);
			return true;
		}

	private:
		usize size_=0;
		usize capacity_;
		T* pool_;
		std::unordered_map<T*,int> ocuupied_indicies_;
		std::vector<int> free_indices_;
		bool present(T* obj) {
			return (ocuupied_indicies_.find(obj) != ocuupied_indicies_.end());
		}

	};
}