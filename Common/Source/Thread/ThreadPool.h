#pragma once

#include <functional>
#include <memory>
#include <queue>
#include <string>
#include <vector>
#include <memory>

#include "Thread.hpp"
#include "Mutex.hpp"
#include "Cond.hpp"

/**
 * @brief A minimal FIFO thread pool.
 *
 * Runs queued tasks on a fixed number of dedicated threads. Tasks are started
 * in the order they were posted and may run in parallel across the pool
 * threads. There is no result value, priority, cancellation or work stealing:
 * a task that needs to report a result must post it back to its owner.
 *
 * Thread safety:
 *   - Post() may be called from any thread, including from a pool task.
 *   - Tasks must not let exceptions escape: they are caught and discarded so
 *     that a pool thread is never terminated.
 *
 * Lifetime:
 *   - The destructor stops accepting wake-ups, runs every task that is still
 *     queued, then joins all threads. It therefore blocks until the running
 *     and queued tasks are finished.
 *   - Tasks must not capture objects that die before the pool. Declare the
 *     pool after every object its tasks use (class members are destroyed in
 *     reverse order of declaration), or capture weak references and lock them
 *     inside the task.
 *   - Post() must not be called after destruction has started.
 *
 * Example:
 *   ThreadPool pool(2);
 *   pool.Post([] {
 *     // slow or blocking work here, then hand the result to its owner
 *   });
 */

class ThreadPool final {
 public:
  explicit ThreadPool(size_t threads = 5) {
    threads_.reserve(threads);
    for (size_t i = 0; i < threads; ++i) {
      auto thread = std::make_unique<ThreadType>(
          "ThreadPool-" + std::to_string(i), &ThreadPool::Run, this);
      thread->Start();
      threads_.push_back(std::move(thread));
    }
  }

  ~ThreadPool() {
    {
      std::lock_guard lock(mutex_);
      stopping_ = true;
    }
    cv_.notify_all();

    // Threads exit only once the queue is empty, so every pending task runs.
    for (auto& thread : threads_) {
      if (thread) {
        thread->Join();
      }
    }
  }

  ThreadPool(const ThreadPool&) = delete;
  ThreadPool& operator=(const ThreadPool&) = delete;

  // Queues a task. Thread-safe. Tasks run in FIFO order, in parallel across
  // the pool threads.
  void Post(std::function<void()> task) {
    {
      std::lock_guard lock(mutex_);
      tasks_.push(std::move(task));
    }
    cv_.notify_one();
  }

 private:
  void Run() {
    // Each thread runs this loop, fetching and executing tasks until stopping_
    // is true and the queue is empty.
    for (;;) {
      std::function<void()> task;
      {
        std::unique_lock lock(mutex_);
        cv_.wait(lock, [this] {
          return stopping_ || !tasks_.empty();
        });

        if (tasks_.empty()) {
          return;  // stopping and nothing left to run
        }

        task = std::move(tasks_.front());
        tasks_.pop();
      }

      // Tasks are expected to handle their own errors; never let an exception
      // escape and terminate the thread.
      try {
        task();
      }
      catch (...) {
      }
    }
  }

  using ThreadType = InvokeThread<decltype(&ThreadPool::Run), ThreadPool*>;


  bool stopping_ = false;

  Mutex mutex_;
  Cond cv_;
  std::queue<std::function<void()>> tasks_;
  std::vector<std::unique_ptr<ThreadType>> threads_;
};
