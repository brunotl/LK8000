/*
 * LK8000 Tactical Flight Computer -  WWW.LK8000.IT
 * Released under GNU/GPL License v.2 or later
 * See CREDITS.TXT file for authors and copyrights
 *
 * File:   DeviceRegistry.cpp
 */
#include "DeviceRegistry.h"
#include "Thread/Mutex.hpp"

void DeviceRegistry::Add(const std::shared_ptr<DeviceSession>& session) {
  const std::lock_guard lock(mutex_);
  sessions_[session->Address()] = session;
}

std::shared_ptr<DeviceSession> DeviceRegistry::Find(std::string_view address) {
  const std::lock_guard lock(mutex_);
  auto it = sessions_.find(std::string(address));
  if (it != sessions_.end()) {
    return it->second;
  }
  return nullptr;
}

std::vector<std::shared_ptr<DeviceSession>> DeviceRegistry::Sessions() {
  const std::lock_guard lock(mutex_);
  std::vector<std::shared_ptr<DeviceSession>> result;
  result.reserve(sessions_.size());
  for (const auto& [_, session] : sessions_) {
    result.push_back(session);
  }
  return result;
}
