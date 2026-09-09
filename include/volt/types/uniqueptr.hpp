#pragma once

#include<volt/memory/PoolAllocator.hpp>
namespace volt {

	template<typename T>
	struct default_delete {
		void operator()(T* ptr) const noexcept {
			delete ptr;
		}
	};



	template<typename T>
	struct pool_delete {
		PoolAllocator* allocator;
		void operator()(T* ptr) const noexcept {
			allocator->deallocate(ptr);
		}
	};
     
	template<
		typename T,
		typename Deleter = default_delete<T>
	>
	class unique_ptr {
	     public:
			 explicit unique_ptr(T* ptr = nullptr) : ptr_(ptr){}

			 explicit unique_ptr(T* ptr, Deleter deleter) : ptr_(ptr), deleter_(deleter) {}
	

			 ~unique_ptr() {
				 deleter_(ptr_);
			 }

			 explicit operator bool() const noexcept {
				 return ptr_ != nullptr;
			 }
			 unique_ptr(const unique_ptr&) = delete;
			 unique_ptr& operator=(const unique_ptr&) = delete;
			 
			 unique_ptr(unique_ptr&& other) noexcept : ptr_(other.ptr_) {
				 ptr_ = other.ptr_;
				 deleter_ = other.deleter_;
				 other.ptr_ = nullptr;
			 }



			 T& operator*() const {
				 return *ptr_;
			 }
			 T* operator->() const noexcept {
				 return ptr_;
			 }

			 unique_ptr& operator=(unique_ptr&& other) noexcept {
				 if (this != &other) {
					 deleter_(ptr_);
					 ptr_ = other.ptr_;
					 other.ptr_ = nullptr;
					 deleter_ = other.deleter_;
				 }
				 return *this;
			 }
			 T* get() const noexcept { return ptr_; }

			 void reset(T* ptr = nullptr) noexcept {
				 deleter_(ptr_);
				 ptr_ = ptr;
				 
			 }

			 T*release() noexcept{
				 T* old_ptr = ptr_;
				 ptr_ = nullptr;
				 return old_ptr;
			 }
			 void swap(unique_ptr& other) noexcept {
				 std::swap(ptr_, other.ptr_);
				 std::swap(deleter_, other.deleter_);
				 
			 }

			
		private:
			 T* ptr_;
			 [[no_unique_address]] Deleter deleter_;
	};

	template<typename T, typename... Args>
	unique_ptr<T> make_unique(Args&&... args) {
		return unique_ptr<T>(new T(std::forward<Args>(args)...));
	}

	
};