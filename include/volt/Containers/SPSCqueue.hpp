#pragma once

#include<cstddef>
#include<atomic>
#include<memory>
#include<bit>
#include <stdexcept>
#include<span>
#include<new>
#include<volt/types/EngineTypes.hpp>
namespace volt {

	
	/// <summary>
	/// A SPSC queue accepts only Power of two size
	/// </summary>
	/// <typeparam name="T"></typeparam>
	template<typename T>
	class SPSCQueue {
	public:
		explicit SPSCQueue(usize capacity) {
			cap = capacity;

			if (!checkPower2(cap)) {
				throw"This is bad size";
			}
			data_ = static_cast<T*>(
				::operator new(
					sizeof(T) * cap,
					std::align_val_t(alignof(T))
					)
				);
		}

		~SPSCQueue() {
			::operator delete(data_, std::align_val_t(alignof(T)));
		}
		SPSCQueue(const SPSCQueue&) = delete;
		SPSCQueue& operator=(const SPSCQueue&) = delete;

		bool push(const T& value) {
			auto tail = tail_.load(std::memory_order_relaxed);

			if (tail - cached_head_ == cap) {
				cached_head_ = head_.load(std::memory_order_acquire);

				if (tail - cached_head_ == cap)
					return false;
			}

			std::construct_at(
				data_ + index(tail),
				value
			);

			tail_.store(tail + 1, std::memory_order_release);
			return true;
		}

		bool push(T&& value) {
			auto tail = tail_.load(std::memory_order_relaxed);

			if (tail - cached_head_ == cap) {
				cached_head_ = head_.load(std::memory_order_acquire);

				if (tail - cached_head_ == cap)
					return false;
			}

			std::construct_at(
				data_ + index(tail),
				std::move(value)
			);

			tail_.store(tail + 1, std::memory_order_release);
			return true;
		}

		template<class... Args>
		bool emplace(Args&&... args) {
			auto tail = tail_.load(std::memory_order_relaxed);

			if (tail - head_.load(std::memory_order_acquire) == cap) {
				return false;
			}

			std::construct_at(
				data_ + index(tail),
				std::forward<Args>(args)...
			);

			tail_.store(tail + 1, std::memory_order_release);
			return true;
		}

		bool pop(T& out) {
			auto head = head_.load(std::memory_order_relaxed);
			if (cached_tail_ == head) {
				cached_tail_ = tail_.load(std::memory_order_acquire);

				if (cached_tail_ == head)
					return false;
			}

			T* element = data_ + index(head);

			out = std::move(*element);
			std::destroy_at(element);

			head_.store(head + 1, std::memory_order_release);
			return true;
		}
		usize size() const {
			return tail_.load() - head_.load();
		}
		bool empty() const noexcept {
			return head_.load() == tail_.load();
		}
		bool full() const noexcept {
			return size() == cap;
		}
		usize capacity() const noexcept {
			return cap;
		}


		usize push_bulk(std::span<const T> values) {
			usize head = head_.load(std::memory_order_relaxed);
			usize tail = tail_.load(std::memory_order_relaxed);
			
			usize used =(tail - head);

			if (used >= cap || values.empty()) {
				return 0;
			}
			const usize available = cap - used;
			const usize count = std::min(values.size(), available);

			for (usize i = 0; i < count; ++i) {
				std::construct_at(
					data_ + index(tail + i),
					values[i]
				);
			}

			tail_.store(
				tail + count,
				std::memory_order_release
			);

			return count;

		}
		usize pop_bulk(std::span<T> output) {
			usize head = head_.load(std::memory_order_relaxed);
			usize tail = tail_.load(std::memory_order_relaxed);

			const usize available = tail - head;
			const usize count = std::min(output.size(), available);

			for (usize i = 0; i < count; ++i) {
				T* element = data_ + index(head + i);
				output[i] = std::move(*element);
				std::destroy_at(element);
			}

			head_.store(
				head + count,
				std::memory_order_release
			);

			return count;
		}
		

		void clear() noexcept {
			auto head = head_.load(std::memory_order_relaxed);
			const auto tail = tail_.load(std::memory_order_acquire);

			while (head != tail) {
				std::destroy_at(data_ + index(head));
				++head;
			}

			head_.store(head, std::memory_order_release);
		}

		T* try_front() noexcept {
			const auto head = head_.load(std::memory_order_relaxed);
			const auto tail = tail_.load(std::memory_order_acquire);

			if (head == tail)
				return nullptr;

			return std::launder(data_ + index(head));
		}

		const T* try_front() const noexcept {
			const auto head = head_.load(std::memory_order_relaxed);
			const auto tail = tail_.load(std::memory_order_acquire);

			if (head == tail)
				return nullptr;

			return std::launder(data_ + index(head));
		}

		T* try_back() noexcept {
			const auto head = head_.load(std::memory_order_relaxed);
			const auto tail = tail_.load(std::memory_order_acquire);

			if (head == tail)
				return nullptr;

			return std::launder(data_ + index(tail - 1));
		}

		const T* try_back() const noexcept {
			const auto head = head_.load(std::memory_order_relaxed);
			const auto tail = tail_.load(std::memory_order_acquire);

			if (head == tail)
				return nullptr;

			return std::launder(data_ + index(tail - 1));
		}

	private:
		bool checkPower2(int x) {
			if (x <= 2) {
				return false;
			}
			return std::has_single_bit(static_cast<unsigned int>(x));
		}
		usize cap;

		T* data_;
		alignas(std::hardware_destructive_interference_size)
			std::atomic<usize> head_{ 0 };

		alignas(std::hardware_destructive_interference_size)
			std::atomic<usize> tail_{ 0 };
		alignas(std::hardware_destructive_interference_size)
			usize cached_head_{};
		alignas(std::hardware_destructive_interference_size)
			usize cached_tail_{};
		
		usize index(usize counter) const noexcept {
			return counter & (cap - 1);
		}

		T* ptr(usize i) noexcept {
			return std::launder(
				reinterpret_cast<T*>(data_) + i
			);
		}

		const T* ptr(usize i) const noexcept {
			return std::launder(
				reinterpret_cast<const T*>(data_) + i
			);
		}
	};

	/// <summary>
	/// A SPSC queue accepts only Power of two size
	/// </summary>
	/// <typeparam name="T"></typeparam>
	/// <typeparam name="siz"></typeparam>
	template<typename T, usize Cap>
		requires (Cap != 0)
	class SPSCQueueStatic {
	public:
		static constexpr usize static_capacity = Cap;
		SPSCQueueStatic() {

		}
		~SPSCQueueStatic() {
			::operator delete(data_, std::align_val_t(alignof(T)));
		}
		SPSCQueueStatic(const SPSCQueueStatic&) = delete;
		SPSCQueueStatic& operator=(const SPSCQueueStatic&) = delete;
		bool push(const T& value) {
			auto tail = tail_.load(std::memory_order_relaxed);

			if (tail - cached_head_ == Cap) {
				cached_head_ = head_.load(std::memory_order_acquire);

				if (tail - cached_head_ == Cap)
					return false;
			}

			data_[index(tail_)] = value;

			tail_.store(tail + 1, std::memory_order_release);
			return true;
		}

		bool push(T&& value) {
			auto tail = tail_.load(std::memory_order_relaxed);

			if (tail - cached_head_ == Cap) {
				cached_head_ = head_.load(std::memory_order_acquire);

				if (tail - cached_head_ == Cap)
					return false;
			}

			data_[index(tail_)] = value;

			tail_.store(tail + 1, std::memory_order_release);
			return true;
		}

		template<class... Args>
		bool emplace(Args&&... args) {
			auto tail = tail_.load(std::memory_order_relaxed);

			if (tail - head_.load(std::memory_order_acquire) == Cap) {
				return false;
			}

			data_[index(tail_)] = std::construct_at(
				data_ + index(tail),
				std::forward<Args>(args)...
			);

			tail_.store(tail + 1, std::memory_order_release);
			return true;
		}

		bool pop(T& out) {
			auto head = head_.load(std::memory_order_relaxed);
			if (cached_tail_ == head) {
				cached_tail_ = tail_.load(std::memory_order_acquire);

				if (cached_tail_ == head)
					return false;
			}

			T* element = data_ + index(head);

			out = std::move(*element);
			std::destroy_at(element);

			head_.store(head + 1, std::memory_order_release);
			return true;
		}

		usize size() const noexcept {
			return tail_.load() - head_.load();
		}

		bool empty() const noexcept {
			return head_.load() == tail_.load();
		}

		bool full() const noexcept {
			return size() == Cap;
		}

		usize capacity() const noexcept {
			return Cap;
		}
		void clear() noexcept {
			auto head = head_.load(std::memory_order_relaxed);
			const auto tail = tail_.load(std::memory_order_acquire);

			while (head != tail) {
				std::destroy_at(data_ + index(head));
				++head;
			}

			head_.store(head, std::memory_order_release);
		}

		usize push_bulk(std::span<const T> values) {
			usize head = head_.load(std::memory_order_relaxed);
			usize tail = tail_.load(std::memory_order_relaxed);

			usize used = (tail - head);

			if (used >= Cap || values.empty()) {
				return 0;
			}
			const usize available = Cap - used;
			const usize count = std::min(values.size(), available);

			for (usize i = 0; i < count; ++i) {
				std::construct_at(
					ptr(index(tail)),
					values[i]
				);
			}

			tail_.store(
				tail + count,
				std::memory_order_release
			);

			return count;

		}
		usize pop_bulk(std::span<T> output) {
			usize head = head_.load(std::memory_order_relaxed);
			usize tail = tail_.load(std::memory_order_relaxed);

			const usize available = tail - head;
			const usize count = std::min(output.size(), available);

			for (usize i = 0; i < count; ++i) {
				T* element = data_ + index(head + i);
				output[i] = std::move(*element);
				std::destroy_at(element);
			}

			head_.store(
				head + count,
				std::memory_order_release
			);

			return count;
		}

		T* try_front() noexcept {
			const auto head = head_.load(std::memory_order_relaxed);
			const auto tail = tail_.load(std::memory_order_acquire);

			if (head == tail)
				return nullptr;

			return std::launder(data_ + index(head));
		}

		const T* try_front() const noexcept {
			const auto head = head_.load(std::memory_order_relaxed);
			const auto tail = tail_.load(std::memory_order_acquire);

			if (head == tail)
				return nullptr;

			return std::launder(data_ + index(head));
		}

		T* try_back() noexcept {
			const auto head = head_.load(std::memory_order_relaxed);
			const auto tail = tail_.load(std::memory_order_acquire);

			if (head == tail)
				return nullptr;

			return std::launder(data_ + index(tail - 1));
		}

		const T* try_back() const noexcept {
			const auto head = head_.load(std::memory_order_relaxed);
			const auto tail = tail_.load(std::memory_order_acquire);

			if (head == tail)
				return nullptr;

			return std::launder(data_ + index(tail - 1));
		}

	private:
		alignas(T) std::byte data_[sizeof(T) * Cap];;

		alignas(std::hardware_destructive_interference_size)
			std::atomic<usize> head_{ 0 };

		alignas(std::hardware_destructive_interference_size)
			std::atomic<usize> tail_{ 0 };

		alignas(std::hardware_destructive_interference_size)
			usize cached_head_;
		alignas(std::hardware_destructive_interference_size)
			usize cached_tail_;
		static constexpr usize index(usize counter) noexcept {
			return counter & (Cap - 1);
		}

		T* ptr(usize i) noexcept {
			return std::launder(
				reinterpret_cast<T*>(data_) + i
			);
		}

		const T* ptr(usize i) const noexcept {
			return std::launder(
				reinterpret_cast<const T*>(data_) + i
			);
		}

		
	};
}