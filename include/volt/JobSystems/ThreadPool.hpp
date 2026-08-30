#pragma once

#include <condition_variable>
#include <functional>
#include <mutex>
#include <queue>
#include <thread>
#include <vector>
#include<volt/types/EngineTypes.hpp>

namespace volt {

	class thread_pool {

		using Task = std::function<void()>;

	public:
		explicit thread_pool(usize worker_count) {
			workers_.reserve(worker_count);

			for (int i = 0; i < worker_count; ++i) {
				workers_.emplace_back([this]{

					WorkerLoop();
					});
			}
		}

		~thread_pool() {
			shutdown();
		}
		

		template<typename F, typename... Args>
		void submit(F&& func, Args&&... args) {
			{ std::lock_guard lock(mutex_);
			if (stopping) {
				return;
			}
			tasks.emplace(
				[func = std::forward<F>(func), ...args = std::forward<Args>(args)]() mutable {
					std::invoke(func,std::forward<decltype(args)>(args)...);
				}
			);

			++pending_count;
			}
			cv_.notify_one();
		}
		void wait() {
			std::unique_lock lock(mutex_);

			finished_cv_.wait(lock, [this]
				{
					return pending_count == 0;
				});
		}

		void shutdown() {
			{
				std::lock_guard lock(mutex_);
				stopping = true;
			}

			cv_.notify_all();

			for (auto& worker : workers_)
			{
				if (worker.joinable())
					worker.join();
			}
		}

	private:
		
		usize pending_count{ 0 };

		std::vector<std::thread> workers_;
		std::mutex mutex_;
		std::condition_variable cv_;
		std::condition_variable finished_cv_;

		std::queue<Task> tasks;
		bool stopping = false;


		
		void WorkerLoop() {
			while (true) {
				Task t;
				
				{
					std::unique_lock lock(mutex_);

					cv_.wait(lock, [this] {
						return stopping || !tasks.empty();
						});
					if (stopping && tasks.empty()) {
						return;
					}
					t = std::move(tasks.front());
					tasks.pop();
				}

				t();
				{
					std::lock_guard lock(mutex_);
					--pending_count;

					if (pending_count == 0) {
						finished_cv_.notify_all();
					}
				}
				
			}
		}
	};
}
