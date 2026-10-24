/*
 * LK8000 Tactical Flight Computer -  WWW.LK8000.IT
 * Released under GNU/GPL License v.2 or later
 * See CREDITS.TXT file for authors and copyrights
 *
 * File:   BlueZLeScanner.cpp
 */
#include "BlueZLeScanner.h"

#include <iterator>
#include <memory>
#include <stdexcept>
#include <string>
#include <utility>

#include "Common.h"
#include "BleManager.h"
#include "AdapterProxy.h"
#include "DeviceProxy.h"

class WndForm;

#define Log(...) BlueZDebugLog(_T("[BluetoothLeScanner]") __VA_ARGS__)

using namespace bluez;

BluetoothLeScanner::BluetoothLeScanner(WndForm* pWndForm, callback_t callback)
    : _pWndForm(pWndForm), _callback(std::move(callback)) {
  try {

    auto& manager = BleManager::Instance();
    auto& runtime = manager.Runtime();
    auto& scanner = manager.Scanner();

    auto connection = runtime.Connection();
    if (!connection) {
      throw std::runtime_error("Failed to get D-Bus connection");
    }

    device_found_subscription_ =
        scanner->SubscribeDeviceFound([this](const DeviceInfo& device) {
          if (SubscribeDevice(device)) {
            UpdateDeviceList(device.path());
          }
        });

    for (auto& device : scanner->Devices()) {
      if (SubscribeDevice(device)) {
        UpdateDeviceList(device.path());
      }
    }

    // Find the first Bluetooth adapter
    auto adapter_path = FindAdapterPath(GetManagedObjects(connection));
    if (adapter_path.empty()) {
      throw std::runtime_error(
          "No Bluetooth adapter found; Bluetooth may be disabled");
    }
    auto adapter_prefix = std::string(adapter_path) + "/";

    // Create proxy for the adapter
    adapter_proxy_ = std::make_shared<AdapterProxy>(connection, adapter_path);

    adapter_proxy_->SetDiscoveryFilter({{"Transport", "le"}});

    // Start discovery
    try {
      adapter_proxy_->StartDiscovery();
      discovery_started = true;
    }
    catch (const std::exception& e) {
      Log("Error starting discovery: %s", e.what());
    }

    Log("Discovery started successfully");
  }
    catch (const std::exception& e) {
    Log("BluetoothLeScanner constructor failed: %s", e.what());
  }
}

BluetoothLeScanner::~BluetoothLeScanner() {

  // Reset the device found subscription before tearing down the scanner.
  device_found_subscription_.Reset();

  if (discovery_started) {
    try {
      adapter_proxy_->StopDiscovery();
    }
    catch (const std::exception& e) {
      Log("Error stopping discovery: %s", e.what());
    }
    discovery_started = false;
  }

  // Detach the signal subscriptions first so no callback can re-enter while the
  // device entries are being torn down.
  auto devices_to_drop = WithLock(devices_mutex, [&] {
    return std::exchange(devices, {});
  });

  for (auto& [path, entry] : devices_to_drop) {
    entry.clear();
  }

  adapter_proxy_.reset();
}

void BluetoothLeScanner::UpdateDeviceList(const DBus::Path& path, std::function<void(DeviceEntry&)> update) {
  std::lock_guard<Mutex> lock(devices_mutex);

  auto it = devices.find(path);
  if (it == devices.end()) {
    return;
  }
  auto& entry = it->second;

  if (update) {
    update(entry);
  }

  const auto* address = entry.address().c_str();
  const auto* name = entry.name().c_str();

  if (entry.is_ble() || !entry.spp_profile()) {
    _callback(_pWndForm, address, name, false);
  }

  if (entry.spp_profile() || !entry.is_ble()) {
    _callback(_pWndForm, address, name, true);
  }
}

bool BluetoothLeScanner::SubscribeDevice(const DeviceInfo& device) {
  auto& manager = BleManager::Instance();
  auto& runtime = manager.Runtime();
  auto connection = runtime.Connection();
  if (!connection) {
    Log("SubscribeDevice: no D-Bus connection for device %s", device.address().c_str());
    return false;
  }

  {
    // Reserve the entry first: this can be reached from the constructor's
    // thread and from the dispatcher thread at the same time.
    const std::lock_guard lock(devices_mutex);
    if (!devices.try_emplace(device.path(), device).second) {
      return false;
    }
  }
  // At this point, the device entry is reserved, but no proxy or monitor is set yet.
  std::shared_ptr<DeviceProxy> proxy;
  std::vector<Subscription> subscriptions;
  try {
    // No lock held: this makes blocking D-Bus calls (AddMatch).
    proxy = std::make_shared<DeviceProxy>(connection, device.path());

    auto it_subs = std::back_inserter(subscriptions);

    it_subs = proxy->SubscribeProperty(
        "Name", [this, path = device.path()](const DBus::Variant& name) {
          UpdateDeviceList(path, [&](DeviceEntry& info) {
            auto new_name = name.to_string();
            if (!new_name.empty()) {
              info.name(new_name);
            }
          });
        });

    it_subs = proxy->SubscribeProperty(
        "Alias", [this, path = device.path()](const DBus::Variant& alias) {
          UpdateDeviceList(path, [&](DeviceEntry& info) {
            info.alias(alias.to_string());
          });
        });

    it_subs = proxy->SubscribeProperty(
        "UUIDs", [this, path = device.path()](const DBus::Variant& uuids) {
          UpdateDeviceList(path, [&](DeviceEntry& info) {
            info.spp_profile(HasSppProfile(uuids));
          });
        });

    it_subs = proxy->SubscribeProperty(
        "AdvertisingFlags", [this, path = device.path()](const DBus::Variant&) {
          UpdateDeviceList(path, [&](DeviceEntry& info) {
            info.is_ble(true);
          });
        });

    it_subs = proxy->SubscribeProperty(
        "ManufacturerData", [this, path = device.path()](const DBus::Variant&) {
          UpdateDeviceList(path, [&](DeviceEntry& info) {
            info.is_ble(true);
          });
        });

    it_subs = proxy->SubscribeProperty(
        "ServiceData", [this, path = device.path()](const DBus::Variant&) {
          UpdateDeviceList(path, [&](DeviceEntry& info) {
            info.is_ble(true);
          });
        });

    const std::lock_guard lock(devices_mutex);
    auto it = devices.find(device.path());
    if (it != devices.end()) {
      it->second.proxy = std::move(proxy);
      it->second.subscriptions = std::move(subscriptions);
    }
    return true;
  }
  catch (const std::exception& e) {
    Log("%s : Error subscribing to device %s", device.address().c_str(), e.what());

    const std::lock_guard lock(devices_mutex);
    devices.erase(device.path());

    return false;
  }
}
