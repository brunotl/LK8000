/*
 * LK8000 Tactical Flight Computer -  WWW.LK8000.IT
 * Released under GNU/GPL License v.2 or later
 * See CREDITS.TXT file for authors and copyrights
 *
 * File:   DeviceProxy.cpp
 */
#include "DeviceProxy.h"
#include <memory>
#include <mutex>
#include "DeviceMonitor.h"
#include "CharacteristicProxy.h"
#include "Common.h"
#include "MessageLog.h"
#include "VariantToString.h"

#define Log(...) BlueZDebugLog(_T("[DeviceProxy]") __VA_ARGS__)

using namespace bluez;

DeviceProxy::DeviceProxy(std::shared_ptr<DBus::Connection> connection,
                         object_path path)
    : Proxy(CreateProxy(connection, kBluezServiceName, path)),
      device_path_(std::move(path)) {
  monitor_ = std::make_unique<DeviceMonitor>(*this);
}

DeviceProxy::~DeviceProxy() {
  Log("%s : Destroying DeviceProxy", device_path_.c_str());
}

void DeviceProxy::Connect() {
  auto method = CreateMethod<void()>("Connect");
  (*method)();
}

void DeviceProxy::Disconnect() {
  auto method = CreateMethod<void()>("Disconnect");
  (*method)();
}

bool DeviceProxy::IsConnected() const {
  auto prop = CreateProperty<bool>("Connected");
  return prop->value();
}

bool DeviceProxy::ServicesResolved() const {
  auto prop = CreateProperty<bool>("ServicesResolved");
  return prop->value();
}

std::optional<uint32_t> DeviceProxy::Class() const {
  try {
    auto prop = CreateProperty<uint32_t>("Class");
    return prop->value();
  }
  catch (const std::exception& e) {
    Log("%s : %s", device_path_.c_str(), e.what());
  }
  return {};
}
  
std::optional<std::string> DeviceProxy::Name() const {
  try {
    auto prop = CreateProperty<std::string>("Name");
    return prop->value();
  }
  catch (const std::exception& e) {
    Log("%s : %s", device_path_.c_str(), e.what());
  }
  return {};
}

void DeviceProxy::DiscoverServices() {
  Log("%s : Discovering services", device_path_.c_str());

  // Helper lambda: check if path belongs to current device
  auto belongsToDevice = [this](const object_path& path) {
    return bluez::StartsWith(path, device_path_);
  };

  auto objects = GetManagedObjects();

  // Collect services for this device
  std::map<object_path, uuid_t> service_uuids;
  for (const auto& [path, interfaces] : objects) {
    auto svc = interfaces.find(kGattServiceInterface);
    if (svc != interfaces.end() && belongsToDevice(path)) {
      service_uuids.try_emplace(path, bluez::GetUUID(svc->second));
    }
  }

  // Discover characteristics and cache their paths
  for (const auto& [path, interfaces] : objects) {
    auto it = interfaces.find(kGattCharacteristicInterface);
    if (it == interfaces.end() || !belongsToDevice(path)) {
      continue;
    }

    // Cache characteristic path for future lookups
    auto service = std::ranges::find_if(service_uuids, [&](const auto& p) {
      return bluez::StartsWith(path, p.first);
    });

    if (service != service_uuids.end()) {
      const uuid_t char_uuid = bluez::GetUUID(it->second);
      const auto mtu = bluez::GetMTU(it->second);

      auto ib = WithLock(characteristic_cache_mutex_, [&] {
        return characteristic_cache_.insert({{service->second, char_uuid}, {path, mtu.value_or(23)}});
      });

      if (ib.second) {
        const auto flags = bluez::GetFlags(it->second);        
        discovered_characteristics_(service->second, char_uuid, flags);
      }
    }
  }
}

DeviceProxy::managed_objects DeviceProxy::GetManagedObjects() const {
  // Get all managed objects from the BlueZ root object.
  // no need to check for null connection, bluez::GetManagedObjects will handle it
  return bluez::GetManagedObjects(GetConnection());
}

DeviceProxy::cache_entry DeviceProxy::FindCharacteristic(
    const uuid_t& service, const uuid_t& characteristic) {
  const auto key = std::make_pair(service, characteristic);

  const std::lock_guard lock(characteristic_cache_mutex_);
  auto cached = characteristic_cache_.find(key);
  if (cached != characteristic_cache_.end()) {
    return cached->second;
  }

  // characteristic_cache_ is populated by the discovery process when device is
  // connected, so if the characteristic is not found here, it means it does not
  // exist

  throw std::runtime_error("Characteristic not found");
}

std::shared_ptr<CharacteristicProxy> DeviceProxy::Characteristic(
    const uuid_t& service, const uuid_t& characteristic) {
  const auto cached = FindCharacteristic(service, characteristic);
  return GetCharacteristicProxy(cached.path, cached.mtu);
}

std::shared_ptr<CharacteristicProxy> DeviceProxy::GetCharacteristicProxy(
    const object_path& path, uint16_t mtu) {
  auto connection = GetConnection();
  if (!connection) {
    throw std::runtime_error("DBus connection Unavailable");
  }

  const std::lock_guard lock(characteristic_proxies_mutex_);

  auto found = characteristic_proxies_.find(path);

  if (found != characteristic_proxies_.end()) {
    return found->second;
  }

  auto proxy = std::make_shared<CharacteristicProxy>(connection, path, mtu);

  auto [it, inserted] = characteristic_proxies_.emplace(path, std::move(proxy));

  return it->second;
}
