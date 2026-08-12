#pragma once
#include<iterator>
#include<vector>



namespace volt{

    /// <summary>
	///  A Simple Fixed Ring Buffer with arbitrary Data storage type.
    /// </summary>
    /// <typeparam name="T"></typeparam>
    template<typename T,std::size_t cap>
    class RingBuffer{
     public:
     RingBuffer(){
   
     }
     ~RingBuffer() {
         clear();
         ::operator delete(data_);
     }

     void push(T val){
         if (full())
             throw std::overflow_error("RingBuffer is full");

         data_[tail_] = value;
         ++tail_
         ++size_;
     }
     void pop() {
         if (empty())
             throw std::out_of_range("RingBuffer is empty");

         std::destroy_at(data_ + head_);

         head_ = next(head_);
         --size_;
     }
     
     inline T front(){
		 return data_[head_];
     }
     inline T back(){
		 return data_[tail_];
     }
  
     void clear() {
         while (size_ > 0) {
             pop();
         }
	 }
     inline bool empty() const {
         return size_ == 0;
     }

     inline bool full() const {
         return size_ == cap;
     }

    inline std::size_t size() const {
         return size_;
     }

    static constexpr std::size_t capacity() noexcept {
        return cap;
    }
    static constexpr std::size_t next(std::size_t index) noexcept {
        return (index + 1) % cap;
    }

    static constexpr std::size_t previous(std::size_t index) noexcept {
        return index == 0 ? cap - 1 : index - 1;
    }
     private:
         T data_[cap];
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
    }; 
    template<typename T, std::size_t cap>
    class RingBufferLogicalPop {
    public:
        RingBuffer() {

        }
        ~RingBuffer() {
            while (!empty()) {
                std::destroy_at(data_ + head_);
            }
            ::operator delete(data_);
        }

        void push(T val) {
            if (full())
                throw std::overflow_error("RingBuffer is full");

            data_[tail_] = value;
            ++tail_
                ++size_;
        }
        void pop() {
            if (empty())
                throw std::out_of_range("RingBuffer is empty");
            --size_;
            head_ = next(head_);
        }

        inline T front() {
            return data_[head_];
        }
        inline T back() {
            return data_[tail_];
        }

        void clear() {
            while (size_ > 0) {
                pop();
            }
        }
        inline bool empty() const {
            return size_ == 0;
        }

        inline bool full() const {
            return size_ == cap;
        }

        inline std::size_t size() const {
            return size_;
        }

        static constexpr std::size_t capacity() noexcept {
            return cap;
        }
        static constexpr std::size_t next(std::size_t index) noexcept {
            return (index + 1) % cap;
        }

        static constexpr std::size_t previous(std::size_t index) noexcept {
            return index == 0 ? cap - 1 : index - 1;
        }
    private:
        T data_[cap];
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
    };
}