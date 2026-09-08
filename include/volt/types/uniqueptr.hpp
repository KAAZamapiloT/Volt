#pragma once


namespace volt {


     
	template<typename T>
	class unique_ptr {
	     public:
			 explicit unique_ptr(T* ptr = nullptr) : ptr_(ptr) {}

			 ~unique_ptr() {
				 delete ptr_;
			 }

			 explicit operator bool() const {
				 return ptr_ != nullptr;
			 }
			 unique_ptr(const unique_ptr&) = delete;
			 unique_ptr& operator=(const unique_ptr&) = delete;
			 
			 unique_ptr(unique_ptr&& other) noexcept : ptr_(other.ptr_) {
				 other.ptr_ = nullptr;
			 }



			 T& operator*() const {
				 return *ptr_;
			 }
			 T* operator->() const noexcept {
				 return ptr_;
			 }


			 T* get() const { return ptr_; }
			 T* reset(T* ptr = nullptr) {
	             T* old_ptr = ptr_;
				 ptr_ = ptr;
				 return old_ptr;
			 }
			 T*release() {
				 T* old_ptr = ptr_;
				 ptr_ = nullptr;
				 return old_ptr;
			 }
			 void swap(UniquePtr& other) noexcept {
				 std::swap(ptr_, other.ptr_);
			 }

		private:
			T* ptr_;
	};

};