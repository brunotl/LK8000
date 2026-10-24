/*
 * LK8000 Tactical Flight Computer -  WWW.LK8000.IT
 * Released under GNU/GPL License v.2 or later
 * See CREDITS.TXT file for authors and copyrights
 *
 * File:   DeviceRegistry.h
 */
#pragma once

#include <memory>
#include <string>
#include <string_view>
#include <unordered_map>
#include <vector>

#include "DeviceSession.h"

/**
 * @brief Registry for managing Bluetooth device sessions.
 *
 * The DeviceRegistry class provides methods to add, find, and retrieve
 * active Bluetooth device sessions. It maintains an internal mapping of
 * device addresses to their corresponding DeviceSession instances.
 */
class DeviceRegistry {
 public:
  void Add(const std::shared_ptr<DeviceSession>& session);

  std::shared_ptr<DeviceSession> Find(std::string_view address);

  std::vector<std::shared_ptr<DeviceSession>> Sessions();

 private:
  Mutex mutex_;
  std::unordered_map<std::string, std::shared_ptr<DeviceSession>> sessions_;
};