/*
 * LK8000 Tactical Flight Computer -  WWW.LK8000.IT
 * Released under GNU/GPL License v.2 or later
 * See CREDITS.TXT file for authors and copyrights
 *
 * File:   DBusRuntime.cpp
 */
#include "DBusRuntime.h"
#include <dbus-cxx.h>
#include <dbus-cxx/simplelogger_defs.h>
#include "Common.h"
#include "OS/ByteOrder.hpp"

#include <sys/prctl.h>



#define Log(...) BlueZDebugLog(_T("[DBusRuntime]") __VA_ARGS__)

DBusRuntime::DBusRuntime() {

#ifdef DEBUG_BLUETOOTH  
  DBus::set_logging_function(
      [](const char* logger_name, const struct ::SL_LogLocation* location,
         const enum ::SL_LogLevel level, const char* log_string) {
        if (level < SL_INFO) {
          return;
        }
        const char* level_str;
        SL_LOGLEVEL_TO_STRING(level_str, level);
        Log("%s %s %s", level_str, logger_name, log_string);
      });

  DBus::set_log_level(SL_TRACE);
#endif

  dispatcher_ = DBus::StandaloneDispatcher::create();
  connection_ = dispatcher_->create_connection(DBus::BusType::SYSTEM);
  worker_ = std::thread(&DBusRuntime::Worker, this);

}

DBusRuntime::~DBusRuntime() {
  {
    std::lock_guard lock(mutex_);
    stopping_ = true;
  }

  cv_.notify_all();

  if (worker_.joinable()) {
    worker_.join();
  }

  connection_.reset();
  dispatcher_.reset();
}

void DBusRuntime::Post(Task task) {
  {
    std::lock_guard lock(mutex_);
    tasks_.push(std::move(task));
  }

  cv_.notify_one();
}

void DBusRuntime::PostDelayed(std::chrono::milliseconds delay, Task task) {
  {
    std::lock_guard lock(mutex_);

    auto deadline = std::chrono::steady_clock::now() + delay;
    delayed_tasks_.emplace(deadline, next_seq_++, std::move(task));
    ++delayed_generation_;
  }

  cv_.notify_one();
}

void DBusRuntime::Worker() {
  prctl(PR_SET_NAME, "DBusRuntime");

  while (true) {
    Task task;

    {
      std::unique_lock lock(mutex_);

      const auto now = std::chrono::steady_clock::now();
      while (!delayed_tasks_.empty() && delayed_tasks_.top().deadline <= now) {
        tasks_.push(delayed_tasks_.pop().task);
      }

      const uint64_t seen_generation = delayed_generation_;
      auto wait_condition = [this, seen_generation] {
        // wait for :
        //   1. stopping_ becomes true
        //   2. a new immediate task is posted
        //   3. a delayed task was posted/changed since we started waiting
        return stopping_ || !tasks_.empty() || delayed_generation_ != seen_generation;
      };

      if (tasks_.empty()) {
        if (delayed_tasks_.empty()) {
          // wait indefinitely if no delayed tasks are scheduled
          cv_.wait(lock, wait_condition);
        }
        else {
          // wait until the next delayed task's deadline or until a new task is posted
          auto deadline = delayed_tasks_.top().deadline;
          cv_.wait_until(lock, deadline, wait_condition);
        }
      }

      if (stopping_) {
        delayed_tasks_.clear(); // discard all delayed tasks when stopping
        if (tasks_.empty()) {
          return;
        }
      }

      if (!tasks_.empty()) {
        task = std::move(tasks_.front());
        tasks_.pop();
      }
    }

    if (task) {
      try {
        task();
      }
      catch (std::exception& e) {
        Log("Exception caught in DBusRuntime worker: %s", e.what());
      }
      catch (...) {
        Log("Unknown exception caught in DBusRuntime worker");
      }
    }
  }
}
