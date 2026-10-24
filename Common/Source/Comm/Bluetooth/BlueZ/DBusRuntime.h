/*
 * LK8000 Tactical Flight Computer -  WWW.LK8000.IT
 * Released under GNU/GPL License v.2 or later
 * See CREDITS.TXT file for authors and copyrights
 *
 * File:   DBusRuntime.h
 */
#pragma once

#include <dbus-cxx.h>

#include <atomic>
#include <chrono>
#include <functional>
#include <memory>
#include <queue>
#include <thread>

#include "Thread/Mutex.hpp"
#include "Thread/Cond.hpp"
#include "utils/move_priority_queue.h"

/**
 * @brief Manages a D-Bus runtime environment with a dedicated worker thread.
 *
 * The DBusRuntime class provides a convenient way to interact with the D-Bus
 * system using a standalone dispatcher and connection. It allows posting tasks
 * to be executed immediately or after a specified delay, ensuring thread-safe
 * execution within the context of the worker thread.
 */
class DBusRuntime
{
public:
    using Task = std::function<void()>;

    DBusRuntime();

    ~DBusRuntime();

    DBusRuntime(const DBusRuntime&) = delete;
    DBusRuntime& operator=(const DBusRuntime&) = delete;

    [[nodiscard]]
    const std::shared_ptr<DBus::Connection>& Connection() const noexcept {
      return connection_;
    }

    [[nodiscard]]
    std::shared_ptr<DBus::StandaloneDispatcher> Dispatcher() const noexcept {
      return dispatcher_;
    }

    void Post(Task task);

    void PostDelayed(std::chrono::milliseconds delay, Task task);

   private:

    void Worker();

    struct DelayedTask {
      std::chrono::steady_clock::time_point deadline;
      uint64_t seq;  // tie-breaker so equal deadlines run in FIFO order
      Task task;

      bool operator>(const DelayedTask& other) const {
        return std::tie(deadline, seq) > std::tie(other.deadline, other.seq);
      }
    };

    mutable Mutex mutex_;
    Cond cv_;

    std::shared_ptr<DBus::StandaloneDispatcher> dispatcher_;
    std::shared_ptr<DBus::Connection> connection_;

    std::thread worker_;

    std::queue<Task> tasks_;

    move_priority_queue<DelayedTask, std::vector<DelayedTask>,
                        std::greater<DelayedTask>>
        delayed_tasks_;  // min-heap via std::greater<>

    uint64_t next_seq_ = 0;  // guarded by mutex_
    uint64_t delayed_generation_ = 0;

    bool stopping_ = false;
};
