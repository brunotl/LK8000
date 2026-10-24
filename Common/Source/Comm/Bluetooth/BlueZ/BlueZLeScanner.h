/*
 * LK8000 Tactical Flight Computer -  WWW.LK8000.IT
 * Released under GNU/GPL License v.2 or later
 * See CREDITS.TXT file for authors and copyrights
 *
 * File:   BlueZLeScanner.h
 */
#pragma once

#include "options.h"
#include <functional>
#include <map>
#include <memory>
#include <string>
#include <vector>

#include "Thread/Mutex.hpp"
#include "DeviceInfo.h"
#include "DeviceProxy.h"

class WndForm;
class AdapterProxy;

class BluetoothLeScanner final {

  using callback_t = std::function<void(WndForm*, const char*, const char*, bool)>;

 public:
  BluetoothLeScanner() = delete;

  BluetoothLeScanner(WndForm* pWndForm, callback_t callback);
  ~BluetoothLeScanner();

 private:

  // Per-device state. Lives as long as the scanner.
  class DeviceEntry : public DeviceInfo {
   public:
    DeviceEntry() = delete;

    explicit DeviceEntry(const DeviceInfo& info) : DeviceInfo(info) {}

    DeviceEntry(const DeviceEntry&) = delete;
    DeviceEntry& operator=(const DeviceEntry&) = delete;

    DeviceEntry(DeviceEntry&&) = default;
    DeviceEntry& operator=(DeviceEntry&&) = default;

    void clear() {
      subscriptions.clear();
      proxy.reset();
    }

    std::shared_ptr<DeviceProxy> proxy;
    std::vector<Subscription> subscriptions;
  };

  WndForm* _pWndForm;
  callback_t _callback;

  bool SubscribeDevice(const DeviceInfo& device);

  void UpdateDeviceList(const DBus::Path& path, std::function<void(DeviceEntry&)> update = {});

  // Devices seen so far, by object path. Never held across a D-Bus call or a
  // callback invocation.
  Mutex devices_mutex;
  std::map<std::string, DeviceEntry> devices;

  Subscription device_found_subscription_;

  bool discovery_started = false;
  std::shared_ptr<AdapterProxy> adapter_proxy_;
};
