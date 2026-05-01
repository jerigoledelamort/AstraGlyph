#include "core/ThreadPool.hpp"

namespace astraglyph {

ThreadPool::ThreadPool(std::size_t numThreads)
{
  start(numThreads);
}

ThreadPool::~ThreadPool()
{
  stop();
}

void ThreadPool::start(std::size_t numThreads)
{
  stop();
  stop_ = false;
  workers_.reserve(numThreads);
  for (std::size_t i = 0; i < numThreads; ++i) {
    workers_.emplace_back(&ThreadPool::workerLoop, this);
  }
}

void ThreadPool::stop()
{
  {
    std::unique_lock<std::mutex> lock(mutex_);
    stop_ = true;
  }
  cv_.notify_all();
  for (auto& worker : workers_) {
    if (worker.joinable()) {
      worker.join();
    }
  }
  workers_.clear();
}

void ThreadPool::enqueue(std::function<void()> task)
{
  {
    std::unique_lock<std::mutex> lock(mutex_);
    tasks_.push(std::move(task));
    ++activeTasks_;
  }
  cv_.notify_one();
}

void ThreadPool::waitAll()
{
  std::unique_lock<std::mutex> lock(mutex_);
  doneCv_.wait(lock, [this] { return activeTasks_ == 0 && tasks_.empty(); });
}

std::size_t ThreadPool::size() const noexcept
{
  return workers_.size();
}

void ThreadPool::workerLoop()
{
  while (true) {
    std::function<void()> task;
    {
      std::unique_lock<std::mutex> lock(mutex_);
      cv_.wait(lock, [this] { return stop_ || !tasks_.empty(); });
      if (stop_ && tasks_.empty()) {
        return;
      }
      task = std::move(tasks_.front());
      tasks_.pop();
    }
    task();
    {
      std::unique_lock<std::mutex> lock(mutex_);
      --activeTasks_;
    }
    doneCv_.notify_all();
  }
}

} // namespace astraglyph
