/*
 * LK8000 Tactical Flight Computer -  WWW.LK8000.IT
 * Released under GNU/GPL License v.2 or later
 * See CREDITS.TXT file for authors and copyrights
 *
 * File:   DeviceMonitor.h
 */
#pragma once

#include "Event.h"

#include <dbus-cxx.h>
#include "Common.h"
#include "PropertyMonitor.h"

#include <memory>
#include <unordered_map>

/**
 * @brief Monitors a Bluetooth device using the BlueZ D-Bus API.
 *
 * The DeviceMonitor class provides functionality to subscribe to property
 * changes of a Bluetooth device. It inherits from PropertyMonitor and is
 * specialized for the BlueZ device interface.
 */
class DeviceMonitor final : public PropertyMonitor {
 public:
  using property_map = bluez::property_map;

  explicit DeviceMonitor(std::shared_ptr<DBus::ObjectProxy> device)
      : PropertyMonitor(device, bluez::kDeviceInterface) {};

  using PropertyMonitor::SubscribeProperty;
};
