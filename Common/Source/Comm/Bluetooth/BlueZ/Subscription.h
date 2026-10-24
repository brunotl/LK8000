/*
 * LK8000 Tactical Flight Computer -  WWW.LK8000.IT
 * Released under GNU/GPL License v.2 or later
 * See CREDITS.TXT file for authors and copyrights
 *
 * File:   Subscription.h
 */
#pragma once

#include <functional>
#include <utility>

// A simple RAII wrapper for managing subscriptions to signals or events.
class Subscription {
 public:
  Subscription() = default;

  explicit Subscription(std::function<void()> unsubscribe)
      : unsubscribe_(std::move(unsubscribe)) {}

  Subscription(const Subscription&) = delete;
  Subscription& operator=(const Subscription&) = delete;

  Subscription(Subscription&& other) noexcept
      : unsubscribe_(std::move(other.unsubscribe_)) {
    other.unsubscribe_ = {};
  }

  Subscription& operator=(Subscription&& other) noexcept {
    if (this != &other) {
      Reset();

      unsubscribe_ = std::move(other.unsubscribe_);

      other.unsubscribe_ = {};
    }

    return *this;
  }

  ~Subscription() {
    Reset();
  }

  void Reset() {
    if (unsubscribe_) {
      unsubscribe_();
      unsubscribe_ = {};
    }
  }

  explicit operator bool() const noexcept {
    return static_cast<bool>(unsubscribe_);
  }

 private:
  std::function<void()> unsubscribe_;
};
