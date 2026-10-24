/*
 * LK8000 Tactical Flight Computer -  WWW.LK8000.IT
 * Released under GNU/GPL License v.2 or later
 * See CREDITS.TXT file for authors and copyrights
 *
 * File:   DeviceScanner.cpp
 */
#include "DeviceScanner.h"
#include <algorithm>
#include <optional>
#include <cassert>

#include "Common.h"
#include "BleManager.h"
#include "Thread/Mutex.hpp"
#include "VariantToString.h"

#include "MessageLog.h"

#define Log(...) BlueZDebugLog(_T("[DeviceScanner]") __VA_ARGS__)

using namespace bluez;

DeviceScanner::~DeviceScanner() {
  Stop();
}

void DeviceScanner::Start(std::shared_ptr<DBus::Connection> connection) {
  if (!connection) {
    throw std::runtime_error("No D-Bus connection");
  }

  root_proxy_ = connection->create_object_proxy(kBluezServiceName, "/");

  interfaces_added_ = SignalConnection<interface_added_signal_type>(
      root_proxy_, kObjectManagerInterface, "InterfacesAdded",
      [weak_this = weak_from_this()](const auto& path, const auto& interfaces) {
        if (auto self = weak_this.lock()) {
          self->HandleInterfacesAdded(path, interfaces);
        }
      });

  interfaces_removed_ = SignalConnection<interface_removed_signal_type>(
      root_proxy_, kObjectManagerInterface, "InterfacesRemoved",
      [weak_this = weak_from_this()](const auto& path, const auto& interfaces) {
        if (auto self = weak_this.lock()) {
          Log("Interfaces removed: %s", path.c_str());
          self->HandleInterfacesRemoved(path, interfaces);
        }
      });

  // Perform an initial scan of existing devices after setting up the signals to
  // avoid missing any devices connecting betwen initialscan and signal setup
  InitialScan();
}

void DeviceScanner::Stop() {
  interfaces_added_.disconnect();
  interfaces_removed_.disconnect();

  auto devices_to_drop = WithLock(mutex_, [&] {
    return std::exchange(devices_, {});
  });

  devices_to_drop.clear();

  root_proxy_.reset();
}

std::vector<DeviceInfo> DeviceScanner::Devices() const {
  std::vector<DeviceInfo> result;
  {
    std::lock_guard lock(mutex_);
    result.reserve(devices_.size());
    for (const auto& [address, info] : devices_) {
      result.push_back(info);
    }
  }
  return result;
}

std::optional<DeviceInfo> DeviceScanner::Find(
    const std::string& address) const {
  std::lock_guard lock(mutex_);
  auto it = devices_.find(address);
  if (it != devices_.end()) {
    return it->second;
  }
  return {};
}

void DeviceScanner::InitialScan() {

  auto objects = bluez::GetManagedObjects(root_proxy_);

  for (const auto& [path, interfaces] : objects) {
    if (!IsDevice(interfaces)) {
      continue;
    }
    try {
      auto info = BuildDevice(path, interfaces);

      std::lock_guard lock(mutex_);
      devices_.insert_or_assign(info.address(), std::move(info));
    }
    catch (const std::exception& e) {
      Log("Error building device for path %s: %s", path.c_str(), e.what());
    }
  }
}

bool DeviceScanner::IsDevice(const interface_map& interfaces) {
  return interfaces.contains(kDeviceInterface);
}

DeviceInfo DeviceScanner::BuildDevice(const DBus::Path& path,
                                      const interface_map& interfaces) {

  const auto& device_props = interfaces.at(kDeviceInterface);

  DeviceInfo info(path, device_props.at("Address").to_string());

  if (auto it = device_props.find("Alias"); it != device_props.end()) {
    info.alias(it->second.to_string());
  }

  if (auto it = device_props.find("Name"); it != device_props.end()) {
    info.name(it->second.to_string());
  }

  if (auto it = device_props.find("Connected"); it != device_props.end()) {
    info.connected(it->second.to_bool());
  }

  if (auto it = device_props.find("Paired"); it != device_props.end()) {
    info.paired(it->second.to_bool());
  }

  if (auto it = device_props.find("Trusted"); it != device_props.end()) {
    info.trusted(it->second.to_bool());
  }

  if (auto it = device_props.find("ServicesResolved"); it != device_props.end()) {
    info.services_resolved(it->second.to_bool());
  }

  if (auto it = device_props.find("UUIDs"); it != device_props.end()) {
    info.spp_profile(HasSppProfile(it->second));
  }

  info.is_ble(device_props.contains("AdvertisingFlags") ||
              device_props.contains("ManufacturerData") ||
              device_props.contains("ServiceData"));

  // TODO: check for Gatt service / properties availability  ??

  return info;
}

void DeviceScanner::HandleInterfacesAdded(const DBus::Path& path,
                                          const interface_map& interfaces) {
  if (!IsDevice(interfaces)) {
    return;
  }

  try {
    auto info = BuildDevice(path, interfaces);

    {
      std::lock_guard lock(mutex_);

      devices_.insert_or_assign(info.address(), info);
    }

    DeviceFound(info);
  }
  catch (const std::exception& e) {
    Log("Error handling interfaces added for path %s: %s", path.c_str(),
        e.what());
  }
}

void DeviceScanner::HandleInterfacesRemoved(
    const DBus::Path& path, const std::vector<std::string>& interfaces) {
  if (std::find(interfaces.begin(), interfaces.end(), kDeviceInterface) ==
      interfaces.end()) {
    // not a device interface, ignore
    return;
  }

  std::optional<DeviceInfo> info_opt;
  {
    std::lock_guard lock(mutex_);

    // find the device by its path, as the address may not be known at this point
    auto it = std::ranges::find_if(devices_, [&](const auto& pair) {
      return pair.second.path() == path;
    });
    if (it != devices_.end()) {
      info_opt = it->second;

      devices_.erase(it);
    }
  }

  if (info_opt) {
    Log("%s / %s : Device removed", info_opt->address().c_str(), info_opt->name().c_str());
    DeviceLost(*info_opt);
  }
}
