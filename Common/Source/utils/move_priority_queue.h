// Copyright (c) 2026, Bruno de Lacheisserie
// All rights reserved.
//
// Redistribution and use in source and binary forms, with or without
// modification, are permitted provided that the following conditions are met:
//
//  * Redistributions of source code must retain the above copyright notice,
//    this list of conditions and the following disclaimer.
//  * Redistributions in binary form must reproduce the above copyright
//    notice, this list of conditions and the following disclaimer in the
//    documentation and/or other materials provided with the distribution.
//  * Neither the name of  nor the names of its contributors may be used to
//    endorse or promote products derived from this software without specific
//    prior written permission.
//
// THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
// AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
// IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
// ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT OWNER OR CONTRIBUTORS BE
// LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
// CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
// SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
// INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
// CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
// ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
// POSSIBILITY OF SUCH DAMAGE.

#pragma once

#include <algorithm>
#include <cassert>
#include <functional>
#include <utility>
#include <vector>

// Not thread-safe: guard with the owner's mutex.
// Same ordering convention as std::priority_queue: top() is the "greatest"
// element according to Compare, so std::greater<T> gives a min-heap.
template <class T, class Container = std::vector<T>,
          class Compare = std::less<typename Container::value_type>>
class move_priority_queue {
 public:
  void push(T&& value) {
    heap_.push_back(std::forward<T>(value));
    std::ranges::push_heap(heap_, comp_);
  }

  template <class... Args>
  void emplace(Args&&... args) {
    heap_.emplace_back(std::forward<Args>(args)...);
    std::ranges::push_heap(heap_, comp_);
  }

  const T& top() const {
    assert(!heap_.empty());
    return heap_.front();
  }

  T pop() {
    assert(!heap_.empty());
    std::ranges::pop_heap(heap_, comp_);
    T value = std::forward<T>(heap_.back());
    heap_.pop_back();
    return value;
  }

  bool empty() const {
    return heap_.empty();
  }

  size_t size() const {
    return heap_.size();
  }

  void clear() {
    heap_.clear();
  }

 private:
  Container heap_;
  [[no_unique_address]] Compare comp_{};
};
