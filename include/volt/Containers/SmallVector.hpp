#pragma once
#include<volt/types/EngineTypes.hpp>
#include <utility>
#include <memory>
#include<assert.h>
namespace volt {
    inline constexpr std::size_t dynamic_extent =
        static_cast<std::size_t>(-1);
    
    template<typename T,usize initial_capacity = dynamic_extent>
    class SmallVector;

/// <summary>
/// A Small Vector with fixed inital capacity bu resizable at runtime if needed.
/// </summary>
/// <typeparam name="T"></typeparam>
/// <typeparam name="initial_size"></typeparam>
/// <typeparam name="max_size"></typeparam>
template<typename T,usize initial_capacity>
class SmallVector {
public:
    SmallVector() :data_(inline_data()), capacity_(initial_capacity) {
    
    }
    ~SmallVector() {
        clear();

        if (!using_inline_storage()) {
            ::operator delete(
                data_,
                std::align_val_t(alignof(T))
                );
        }
    }
    const T& operator[](usize index) const noexcept {
        return data_[index];
    }
    T& operator[](usize index) noexcept {
        return data_[index];
    }
    template<typename U>
    void push_back(U&& value) {
        if (size_ == capacity_) {
            reallocate(capacity_ * 2);
        }

        std::construct_at(
            data_ + size_,
            std::forward<U>(value)
        );

        ++size_;
    }
    void pop_back() noexcept {
        assert(size_ > 0);

        std::destroy_at(data_ + size_ - 1);
        --size_;
    }
     bool empty() noexcept {
		 return size_ == 0;
     }
     void clear() noexcept {
         while (size_ > 0) {
             std::destroy_at(data_ + size_ - 1);
             --size_;
         }
     }
     template<typename... Args>
     T& emplace_back(Args&&... args) {
         if (size_ == capacity_) {
             reallocate(2*size_);
         }
         T* element = data_ + size_;

         std::construct_at(
             element,
             std::forward<Args>(args)...
         );

         ++size_;

         return *element;
     }
     usize size() const noexcept {
         return size_;
	 }
     void reserve(usize new_capacity) {
         if (new_capacity > capacity_) {
             reallocate(new_capacity);
         }
	 }
private:
    T* data_;
	alignas(T) std::byte inline_storage[initial_capacity * sizeof(T)];
    usize capacity_ = initial_capacity;
    usize size_ = 0;

    void reallocate(usize caps) {
      
        T* new_data = static_cast<T*>(
            ::operator new(
                sizeof(T) * caps,
                std::align_val_t(alignof(T))
                )
            );
       
        for(usize i = 0; i < size_; ++i) {
            std::construct_at(
                new_data + i,
                std::move(data_[i])
            );
		}
        for (usize i = 0; i < size_; ++i) {
            std::destroy_at(data_ + i);
        }
        if (!using_inline_storage()) {
            ::operator delete(
                data_,
                std::align_val_t(alignof(T))
                );
        }
        capacity_ = caps;
        data_ = new_data;
    }
    
    T* inline_data() noexcept {
        return reinterpret_cast<T*>(inline_storage);
    }
    const T* inline_data() const noexcept {
        return reinterpret_cast<const T*>(inline_storage);
    }
    bool using_inline_storage() const noexcept {
        return data_ == inline_data();
    }
    T& at(usize index) {
        if (index >= size_) {
            throw std::out_of_range("SmallVector::at");
        }
        return data_[index];
    }
};

}