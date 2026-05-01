#pragma once

#include <condition_variable>
#include <cstddef>
#include <functional>
#include <mutex>
#include <queue>
#include <thread>
#include <vector>

namespace astraglyph {

class ThreadPool {
public:
  ThreadPool() = default;
  explicit ThreadPool(std::size_t numThreads);
  ~ThreadPool();

  ThreadPool(const ThreadPool&) = delete;
  ThreadPool& operator=(const ThreadPool&) = delete;
  ThreadPool(ThreadPool&&) = delete;
  ThreadPool& operator=(ThreadPool&&) = delete;

  void start(std::size_t numThreads);
  void stop();
  void enqueue(std::function<void()> task);
  void waitAll();

  [[nodiscard]] std::size_t size() const noexcept;

private:
  void workerLoop();

  std::vector<std::thread> workers_;
  std::queue<std::function<void()>> tasks_;
  std::mutex mutex_;
  std::condition_variable cv_;
  std::condition_variable doneCv_;
  std::size_t activeTasks_ = 0;
  bool stop_ = false;
};

} // namespace astraglyph
