/*
 * LK8000 Tactical Flight Computer -  WWW.LK8000.IT
 * Released under GNU/GPL License v.2 or later
 * See CREDITS.TXT file for authors and copyrights
 *
 * File:   DeviceScanner.h
 */
#pragma once

#include <dbus-cxx.h>

#include <memory>
#include <mutex>
#include <unordered_map>
#include <vector>

#include "Event.h"
#include "DeviceInfo.h"
#include "Common.h"
#include "SignalConnection.h"

/**
 * @brief Scans for Bluetooth devices using the BlueZ D-Bus API.
 *
 * The DeviceScanner class provides functionality to start and stop scanning
 * for Bluetooth devices, and to subscribe to events when devices are found
 * or lost.
 */
class DeviceScanner : public std::enable_shared_from_this<DeviceScanner> {
 public:
  DeviceScanner() = default;

  DeviceScanner(const DeviceScanner&) = delete;
  DeviceScanner& operator=(const DeviceScanner&) = delete;

  ~DeviceScanner();

  void Start(std::shared_ptr<DBus::Connection> connection);

  void Stop();

  [[nodiscard]]
  std::vector<DeviceInfo> Devices() const;

  [[nodiscard]]
  std::optional<DeviceInfo> Find(const std::string& address) const;

  [[nodiscard]]
  Subscription SubscribeDeviceFound(Event<DeviceInfo>::Handler&& handler) {
    return DeviceFound.Subscribe(std::move(handler));
  }

  [[nodiscard]]
  Subscription SubscribeDeviceLost(Event<DeviceInfo>::Handler&& handler) {
    return DeviceLost.Subscribe(std::move(handler));
  }

 protected:
  Event<DeviceInfo> DeviceFound;
  Event<DeviceInfo> DeviceLost;

 private:
  using managed_objects = bluez::managed_objects;
  using property_map = bluez::property_map;
  using interface_map = bluez::interface_map;

  using interface_added_signal_type = void(DBus::Path, interface_map);
  using interface_removed_signal_type = void(DBus::Path, std::vector<std::string>);

  void InitialScan();

  void HandleInterfacesAdded(const DBus::Path& path,
                             const interface_map& interfaces);

  void HandleInterfacesRemoved(const DBus::Path& path,
                               const std::vector<std::string>& interfaces);

  static bool IsDevice(const interface_map& interfaces);

  static DeviceInfo BuildDevice(const DBus::Path& path,
                                const interface_map& interfaces);

 private:
  // Proxy for the root object ("/") of the BlueZ service
  // must be hold to keep the it alive for signal subscriptions
  std::shared_ptr<DBus::ObjectProxy> root_proxy_;

  mutable Mutex mutex_; // protects access to devices_
  std::unordered_map<std::string, DeviceInfo> devices_;

  SignalConnection<interface_added_signal_type> interfaces_added_;
  SignalConnection<interface_removed_signal_type> interfaces_removed_;
};
