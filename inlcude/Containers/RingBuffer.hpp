#pragma once
#include<iterator>
#include<vector>



namespace volt{

    /// <summary>
	///  A Simple Fixed Ring Buffer with arbitrary Data storage type.
    /// </summary>
    /// <typeparam name="T"></typeparam>
    template<typename T>
    class RingBuffer{
     public:
     explicit RingBuffer(std::size_t capacity):cap(capacity){
         data_ = static_cast<T*>(::operator new(capacity * sizeof(T));

     }
     ~RingBuffer() {
         clear();
         ::operator delete(data_);
     }

     void push(T val){
         if (full())
             throw std::overflow_error("RingBuffer is full");

         std::construct_at(data_ + tail_, value);

         tail_ = next(tail_);
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
         return size_ == capacity_;
     }

    inline std::size_t size() const {
         return size_;
     }

     inline std::size_t capacity() const {
         return capacity_;
     }
     private:
         T* data_;
         std::size_t capacity_;
         std::size_t head_;
         std::size_t tail_;
         std::size_t size_;
        
    };


    class DynamicRingBufer {
    public:
        DynamicRingBufer(std::size_t inital_capcaity = 10):capacity_(inital_capcaity){
			data_ = static_cast<T*>(::operator new(capacity_ * sizeof(T)));
        }
        ~DynamicRingBuffer() {
            while (!empty())
            {
                pop();
            }
        }

        void pop() {

        }

    private:
        T* data_;
		std::size_t capacity_;
		std::size_t head_;
		std::size_t tail_;
		std::size_t size_;

        
    };

    
}