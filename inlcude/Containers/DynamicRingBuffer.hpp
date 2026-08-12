namespace volt {

    /// <summary>
    /// A resizable Ring Buffer With Basic Operations
    /// </summary>
    class DynamicRingBufer {
    public:
        DynamicRingBufer(std::size_t inital_capcaity = 10) :capacity_(inital_capcaity) {
            data_ = static_cast<T*>(::operator new(capacity_ * sizeof(T)));
        }
        ~DynamicRingBuffer() {
            while (!empty())
            {
                pop();
            }
        }

        void pop() {
            if (empty())
                throw std::out_of_range("RingBuffer is empty");

            std::destroy_at(data_ + head_);

            head_ = next(head_);
            --size_;
        }

    private:
        T* data_;
        std::size_t capacity_;
        std::size_t head_;
        std::size_t tail_;
        std::size_t size_;


    };
}