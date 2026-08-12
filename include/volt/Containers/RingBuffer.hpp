#pragma once
#include <cstddef>
#include <memory>
#include <new>
#include <stdexcept>


namespace volt{

    /// <summary>
	///  A Simple Fixed Ring Buffer with arbitrary Data storage type.
    /// </summary>
    /// <typeparam name="T"></typeparam>
    template<typename T,std::size_t cap>
    class RingBuffer {
    public:
        RingBuffer() = default;
        ~RingBuffer() = default;

        void push(const T& value) {
            if (full())
                throw std::overflow_error("RingBuffer is full");

            data_[tail_] = value;
            tail_ = next(tail_);
            ++size_;
        }

        void pop() {
            if (empty())
                throw std::out_of_range("RingBuffer is empty");

            head_ = next(head_);
            --size_;
        }

        T& front() noexcept {
            return data_[head_];
        }

        const T& front() const noexcept {
            return data_[head_];
        }

        T& back() noexcept {
            return data_[previous(tail_)];
        }

        const T& back() const noexcept {
            return data_[previous(tail_)];
        }

        void clear() noexcept {
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
            return size_ == cap;
        }

        [[nodiscard]]
        std::size_t size() const noexcept {
            return size_;
        }

        [[nodiscard]]
std::size_t capacity() const noexcept {
    return cap;
}

    private:
        static constexpr std::size_t next(std::size_t index) noexcept {
            return (index + 1) % cap;
        }

        static constexpr std::size_t previous(std::size_t index) noexcept {
            return index == 0 ? cap - 1 : index - 1;
        }

   
        T data_[cap]{};

        std::size_t head_ = 0;
        std::size_t tail_ = 0;
        std::size_t size_ = 0;
    };

    /// @brief 
    /// This Version creates object at runtime so use this carefully
    /// @tparam T 
    template<typename T>
    class RingBuffer<T, 0> {
    public:
        explicit RingBuffer(std::size_t capacity)
            : capacity_(capacity)
        {
            if (capacity_ == 0)
                throw std::invalid_argument("Capacity must be > 0");

            storage_ = static_cast<std::byte*>(
                ::operator new(sizeof(T) * capacity_)
                );
        }

        ~RingBuffer() {
    clear();
    ::operator delete(storage_);
}

RingBuffer(const RingBuffer&) = delete;
RingBuffer& operator=(const RingBuffer&) = delete;
        void push(const T& value) {
            if (full())
                throw std::overflow_error("RingBuffer is full");
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

        T copy_front() const {
            return *ptr(head_);
        }

        T& back() noexcept {
            return *ptr(previous(tail_));
        }

        const T& back() const noexcept {
            return *ptr(previous(tail_));
        }

        T copy_back() const {
            return *ptr(previous(tail_));
        }

        void clear() noexcept {
            while (!empty())
                pop();
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
        
        std::byte* storage_ = nullptr;
        std::size_t capacity_=10;
        std::size_t head_ = 0;
        std::size_t tail_ = 0;
        std::size_t size_ = 0;
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
            return index == 0
                ? capacity_ - 1
                : index - 1;
        }

    };
}