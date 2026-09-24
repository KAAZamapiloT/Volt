#pragma once
#include<memory>
#include<unordered_set>
#include<set>
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
			for(int i=0;i<capacity_;i++){
				pool_[i] = ::operator new(sizeof(T),align_at_t(alignof(T));
				free_indices_.push_back(i);
			}
		};

		~ObjectPool() {
		};

		[[nodiscard]]
		template<typename... Args>
		T* allocate(Args&&...args) {

			if (free_indices_.empty()) { return; }
			auto free_index = free_indices_.back();
			free_indices_.pop_back();
			pool_[free_index] = new(pool_ + free_index * sizeof(T)) T(std::forward<Args>(args)...);
			++size_;
			return pool_[free_index];
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
			int free_index = ocuupied_indicies_[obj];
			ocuupied_indicies_.erase(obj);
			free_indices_.insert(free_index);
			return true;
		}

	private:
		usize size_;
		usize capacity_;
		std::byte* pool_;
		std::unordered_map<T*,int> ocuupied_indicies_;
		std::vector<int> free_indices_;
		bool present(T* obj) {
			return (ocuupied_indicies_.find(obj) != ocuupied_indicies_.end());
		}

	};
}