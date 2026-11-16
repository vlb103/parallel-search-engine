#include "thread_pool.h"

#include <exception>
#include <iostream>
#include <utility>

ThreadPool::ThreadPool(size_t thread_count) {
  for (size_t i = 0; i < thread_count; ++i) {
    workers_.emplace_back([this] { WorkerLoop(); });
  }
}

ThreadPool::~ThreadPool() { Shutdown(); }

void ThreadPool::Submit(std::function<void()> task) {
  {
    std::lock_guard<std::mutex> lock(mutex_);
    tasks_.push(std::move(task));
  }
  has_work_.notify_one();
}

void ThreadPool::Shutdown() {
  {
    std::lock_guard<std::mutex> lock(mutex_);
    stopping_ = true;
  }
  has_work_.notify_all();
  for (std::thread& worker : workers_) {
    if (worker.joinable()) worker.join();
  }
}

void ThreadPool::WorkerLoop() {
  while (true) {
    std::function<void()> task;
    {
      std::unique_lock<std::mutex> lock(mutex_);
      has_work_.wait(lock, [this] { return stopping_ || !tasks_.empty(); });
      if (tasks_.empty()) return;
      task = std::move(tasks_.front());
      tasks_.pop();
    }
    try {
      task();
    } catch (const std::exception& error) {
      std::cerr << "task failed: " << error.what() << '\n';
    }
  }
}
