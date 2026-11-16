#ifndef SRC_THREAD_POOL_H_
#define SRC_THREAD_POOL_H_

#include <condition_variable>
#include <cstddef>
#include <functional>
#include <mutex>
#include <queue>
#include <thread>
#include <vector>

class ThreadPool {
 public:
  explicit ThreadPool(size_t thread_count);
  ~ThreadPool();

  ThreadPool(const ThreadPool&) = delete;
  ThreadPool& operator=(const ThreadPool&) = delete;

  void Submit(std::function<void()> task);

  void Shutdown();

 private:
  void WorkerLoop();

  std::mutex mutex_;
  std::condition_variable has_work_;
  std::queue<std::function<void()>> tasks_;
  bool stopping_ = false;
  std::vector<std::thread> workers_;
};

#endif  // SRC_THREAD_POOL_H_
