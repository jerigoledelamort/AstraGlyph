#include "core/ThreadPool.hpp"

#include <algorithm>

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
  
  stop_.store(false);
  activeTasks_.store(0);
  threadCount_ = numThreads;
  
  workers_.reserve(numThreads);
  for (std::size_t i = 0; i < numThreads; ++i) {
    workers_.emplace_back(&ThreadPool::workerLoop, this, i);
  }
}

void ThreadPool::stop()
{
  {
    std::unique_lock<std::mutex> lock(mutex_);
    stop_.store(true);
  }
  cv_.notify_all();
  
  for (auto& worker : workers_) {
    if (worker.joinable()) {
      worker.join();
    }
  }
  workers_.clear();
  threadCount_ = 0;
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
  doneCv_.wait(lock, [this] { 
    return activeTasks_.load() == 0 && tasks_.empty(); 
  });
}

std::size_t ThreadPool::size() const noexcept
{
  return workers_.size();
}

std::size_t ThreadPool::threadCount() const noexcept
{
  return threadCount_;
}

void ThreadPool::workerLoop(std::size_t workerId)
{
  (void)workerId;  //可以用于 work stealing в будущем
  
  while (true) {
    std::function<void()> task;
    {
      std::unique_lock<std::mutex> lock(mutex_);
      cv_.wait(lock, [this] { 
        return stop_.load() || !tasks_.empty(); 
      });
      
      if (stop_.load() && tasks_.empty()) {
        return;
      }
      
      if (!tasks_.empty()) {
        task = std::move(tasks_.front());
        tasks_.pop();
      }
    }
    
    if (task) {
      task();
      if (--activeTasks_ == 0) {
        doneCv_.notify_all();
      }
    }
  }
}

} // namespace astraglyph
