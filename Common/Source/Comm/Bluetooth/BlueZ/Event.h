/*
 * LK8000 Tactical Flight Computer -  WWW.LK8000.IT
 * Released under GNU/GPL License v.2 or later
 * See CREDITS.TXT file for authors and copyrights
 *
 * File:   Event.h
 */
#pragma once

#include "Subscription.h"

#include <algorithm>
#include <functional>
#include <memory>
#include <vector>
#include <thread>
#include "Thread/Mutex.hpp"
#include "Thread/Cond.hpp"
#include "Util/ScopeExit.hxx"


/**
 * @brief A thread-safe event class that allows subscribing handlers and notifying them.
 *
 * The Event class allows multiple handlers to be registered. Handlers can be
 * unsubscribed safely even while notifications are in progress. Each handler
 * runs in the context of the thread that calls Notify().
 *
 * @tparam Args The argument types that the event handlers will receive.
 */
template <typename... Args>
class Event {
 public:
  using Handler = std::function<void(Args...)>;


  // Registers a handler. The returned Subscription unsubscribes synchronously:
  // once Reset() (or the destructor) returns, the handler is guaranteed not to
  // be running on any other thread, and it will never be invoked again.
  //
  // Caveat: the Event must outlive every Subscription it has handed out.
  [[nodiscard]]
  Subscription Subscribe(Handler handler) {
    auto entry = std::make_shared<Entry>();

    entry->handler = std::move(handler);

    {
      std::lock_guard lock(mutex_);

      handlers_.push_back(entry);
    }

    return Subscription([this, weak = std::weak_ptr<Entry>(entry)] {
      auto shared = weak.lock();
      if (!shared) {
        return;
      }

      { // Step 1: remove the entry so future Notify() calls no longer see it.
        std::lock_guard lock(mutex_);
        std::erase(handlers_, shared);
      }
      // Step 2: forbid new invocations. This also covers Notify() calls that
      // already took a snapshot containing this entry but have not invoked the
      // handler yet: they will observe `active == false` and skip it.
      std::unique_lock lock(shared->mutex);
      shared->active = false;

      const auto self = std::this_thread::get_id();

      // Step 3: wait for invocations running on *other* threads to finish.
      // Invocations on the current thread are ignored, so a handler can safely
      // unsubscribe itself (the call in progress simply completes normally).
      shared->cv.wait(lock, [&] {
        return std::ranges::none_of(shared->running, 
          [&](std::thread::id id) {
            return id != self;
          });
      });

    });
  }

  // Invokes all active handlers. Handlers run without any lock held, so they
  // can execute concurrently and may freely call Subscribe(), Empty() or
  // Reset() on their own subscription.
  void Notify(Args... args) const {
    // Work on a snapshot so handlers can (un)subscribe during iteration.
    // The shared_ptrs keep each Entry alive until this call is done.
    std::vector<std::shared_ptr<Entry>> handlers;
    {
      std::lock_guard lock(mutex_);
      handlers = handlers_;
    }

    for (const auto& entry : handlers) {
      // Register this invocation, unless the entry was unsubscribed meanwhile.
      {
        std::lock_guard lock(entry->mutex);
        if (!entry->active) {
          continue;
        }
        entry->running.push_back(std::this_thread::get_id());
      }

      // Scope guard to remove the current thread from the running list and
      // notify all waiting threads. This ensures that the `running` list is
      // correctly updated and any threads waiting for this handler to finish
      // are notified, even if exceptions occur. It is implemented using the
      // `AtScopeExit` utility to guarantee cleanup letting exceptions propagate
      // normally.
      AtScopeExit(&) {
        std::lock_guard lock(entry->mutex);

        // Remove the most recent occurrence of this thread's id (matters when
        // Notify() is re-entered on the same thread).
        auto it = std::find(entry->running.rbegin(), entry->running.rend(),
                            std::this_thread::get_id());

        entry->running.erase(std::next(it).base());
        entry->cv.notify_all();
      };

      entry->handler(args...);
    }
  }

  void operator()(Args... args) const {
    Notify(std::forward<Args>(args)...);
  }

  [[nodiscard]]
  bool Empty() const {
    std::lock_guard lock(mutex_);

    return handlers_.empty();
  }

 private:
  struct Entry {
    Handler handler;
    // Protects `active` and `running`. Never held while the handler runs.
    Mutex mutex;
    Cond cv;

    // Set to false on unsubscribe; checked before each invocation.
    bool active = true;

    // One thread id per invocation currently in progress (several entries if
    // Notify() runs in parallel on multiple threads or is re-entered).
    std::vector<std::thread::id> running;
  };

 private:
  mutable Mutex mutex_;

  std::vector<std::shared_ptr<Entry>> handlers_;
};
