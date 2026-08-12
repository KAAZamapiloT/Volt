#pragma once
#include <cstddef>
#include <memory>
#include <new>
#include <stdexcept>
namespace volt {
  
    /// <summary>
    /// A resizable Ring Buffer With Basic Operations use with caution
    /// </summary>

    
    template<typename T>
    class DynamicRingBuffer {
    public:
    
        explicit DynamicRingBuffer(std::size_t capacity = 8)
            : capacity_(capacity)
        {
            if (capacity_ == 0)
                throw std::invalid_argument(
                    "DynamicRingBuffer capacity must be > 0"
                );

            storage_ = static_cast<std::byte*>(
                ::operator new(sizeof(T) * capacity_)
                );
        }

        ~DynamicRingBuffer() {
            clear();
            ::operator delete(storage_);
        }

        DynamicRingBuffer(const DynamicRingBuffer&) = delete;
        DynamicRingBuffer& operator=(const DynamicRingBuffer&) = delete;

        void push(const T& value) {
            if (full())
                resize(capacity_ * 2);
            std::construct_at(ptr(tail_), value);
            tail_ = next(tail_);
            ++size_;
		}
        void pop() {
            if (empty())
                throw std::out_of_range("RingBuffer is empty");
            std::destroy_at(ptr(head_));
            head_ = next(head_);
            --size_;
        }
        T& front() noexcept {
            return *ptr(head_);
        }

        const T& front() const noexcept {
            return *ptr(head_);
        }

        T& back() noexcept {
            return *ptr(previous(tail_));
        }

        const T& back() const noexcept {
            return *ptr(previous(tail_));
        }
        void clear() noexcept {
            for (std::size_t i = 0; i < size_; ++i) {
                std::destroy_at(ptr((head_ + i) % capacity_));
            }
            size_ = 0;
            head_ = 0;
            tail_ = 0;
        }
        [[nodiscard]]
        bool empty() const noexcept {
            return size_ == 0;
        }

        [[nodiscard]]
        bool full() const noexcept {
            return size_ == capacity_;
        }

        [[nodiscard]]
        std::size_t size() const noexcept {
            return size_;
        }

        [[nodiscard]]
        std::size_t capacity() const noexcept {
            return capacity_;
        }

    private:
        std::byte* storage_=nullptr;
        
		std::size_t capacity_ = 10;
        std::size_t head_ = 0;
		std::size_t tail_ = 0;
		std::size_t size_ = 0;

        void resize(std::size_t new_capacity) {

            std::byte* new_storage =
                static_cast<std::byte*>(
                    ::operator new(sizeof(T) * new_capacity)
                    );

            std::size_t constructed = 0;

            try {
                for (std::size_t i = 0; i < size_; ++i) {
                    std::construct_at(
                        reinterpret_cast<T*>(new_storage) + i,
                        std::move_if_noexcept(
                            *ptr((head_ + i) % capacity_)
                        )
                    );

                    ++constructed;
                }
            }
            catch (...) {
                T* new_data =
                    reinterpret_cast<T*>(new_storage);

                for (std::size_t i = 0; i < constructed; ++i)
                    std::destroy_at(new_data + i);

                ::operator delete(new_storage);
                throw;
            }

            for (std::size_t i = 0; i < size_; ++i) {
                std::destroy_at(
                    ptr((head_ + i) % capacity_)
                );
            }

            ::operator delete(storage_);

            storage_ = new_storage;
            capacity_ = new_capacity;
            head_ = 0;
            tail_ = size_;
		}
        T* ptr(std::size_t index) noexcept {
            return std::launder(
                reinterpret_cast<T*>(storage_) + index
            );
        }

        const T* ptr(std::size_t index) const noexcept {
            return std::launder(
                reinterpret_cast<const T*>(storage_) + index
            );
        }
        std::size_t next(std::size_t index) const noexcept {
            return (index + 1) % capacity_;
        }

        std::size_t previous(std::size_t index) const noexcept {
            return index == 0 ? capacity_ - 1 : index - 1;
        }
  

    };
}