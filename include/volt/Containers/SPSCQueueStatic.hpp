#pragma once
#include<volt/types/EngineTypes.hpp>
#include <utility>
#include <memory>
#include<assert.h>

namespace volt{


	/// <summary>
	/// A SPSC queue accepts only Power of two size
	/// </summary>
	/// <typeparam name="T"></typeparam>
	/// <typeparam name="siz"></typeparam>
	template<typename T, usize Cap>
		requires (Cap != 0)
	class SPSCQueueStatic {
	public:
		static_assert(
			Cap > 0 && std::has_single_bit(Cap),
			"SPSCQueueStatic capacity must be a power of two"
			);
		static constexpr usize static_capacity = Cap;
		constexpr SPSCQueueStatic() noexcept = default;
		~SPSCQueueStatic() {
			clear();
		}
		SPSCQueueStatic(const SPSCQueueStatic&) = delete;
		SPSCQueueStatic& operator=(const SPSCQueueStatic&) = delete;
		SPSCQueueStatic(SPSCQueueStatic&&) = delete;
		SPSCQueueStatic& operator=(SPSCQueueStatic&&) = delete;

		bool push(const T& value) {
			auto tail = tail_.load(std::memory_order_relaxed);

			if (tail - cached_head_ == Cap) {
				cached_head_ = head_.load(std::memory_order_acquire);

				if (tail - cached_head_ == Cap)
					return false;
			}

			std::construct_at(
				ptr(index(tail)),
				value
			);

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

			std::construct_at(
				ptr(index(tail)),
				std::move(value)
			);

			tail_.store(tail + 1, std::memory_order_release);
			return true;
		}

	
		template<class... Args>
		bool emplace(Args&&... args) {
			auto tail = tail_.load(std::memory_order_relaxed);

			if (tail - cached_head_ == Cap) {
				cached_head_ =
					head_.load(std::memory_order_acquire);

				if (tail - cached_head_ == Cap)
					return false;
			}

			std::construct_at(
				ptr(index(tail)),
				std::forward<Args>(args)...
			);

			tail_.store(
				tail + 1,
				std::memory_order_release
			);

			return true;
		}

		bool pop(T& out) {
			auto head = head_.load(std::memory_order_relaxed);
			if (cached_tail_ == head) {
				cached_tail_ = tail_.load(std::memory_order_acquire);

				if (cached_tail_ == head)
					return false;
			}

			T* element = ptr(index(head));;

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
				std::destroy_at(ptr(index(head)));
				++head;
			}

			head_.store(head, std::memory_order_release);
		}

		usize push_bulk(std::span<const T> values) {
			if (values.empty())
				return 0;
			auto tail = tail_.load(std::memory_order_relaxed);

			if (tail - cached_head_ >= Cap) {
				cached_head_ =
					head_.load(std::memory_order_acquire);

				if (tail - cached_head_ >= Cap)
					return 0;
			}
			const usize available = Cap - (tail - cached_head_);
			const usize count = std::min(values.size(), available);

			for (usize i = 0; i < count; ++i) {
				std::construct_at(
					ptr(index(tail + i)),
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
			auto head = head_.load(std::memory_order_relaxed);

			if (cached_tail_ == head) {
				cached_tail_ = tail_.load(std::memory_order_acquire);

				if (cached_tail_ == head)
					return 0;
			}

			const usize available = cached_tail_ - head;
			const usize count = std::min(output.size(), available);

			for (usize i = 0; i < count; ++i) {
				T* element = ptr(index(head + i));
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

			return ptr(index(head));
		}

		const T* try_front() const noexcept {
			const auto head = head_.load(std::memory_order_relaxed);
			const auto tail = tail_.load(std::memory_order_acquire);

			if (head == tail)
				return nullptr;

			return ptr(index(head));
		}

		T* try_back() noexcept {
			const auto head = head_.load(std::memory_order_relaxed);
			const auto tail = tail_.load(std::memory_order_acquire);

			if (head == tail)
				return nullptr;

			return ptr(index(tail - 1));
		}

		const T* try_back() const noexcept {
			const auto head = head_.load(std::memory_order_relaxed);
			const auto tail = tail_.load(std::memory_order_acquire);

			if (head == tail)
				return nullptr;

			return ptr(index(tail -1));;
		}

	private:
		alignas(T) std::byte data_[sizeof(T) * Cap];;

		alignas(std::hardware_destructive_interference_size)
			std::atomic<usize> head_{ 0 };

		alignas(std::hardware_destructive_interference_size)
			std::atomic<usize> tail_{ 0 };

		alignas(std::hardware_destructive_interference_size)
			usize cached_head_{};
		alignas(std::hardware_destructive_interference_size)
			usize cached_tail_{};
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