/*
 * LK8000 Tactical Flight Computer -  WWW.LK8000.IT
 * Released under GNU/GPL License v.2 or later
 * See CREDITS.TXT file for authors and copyrights
 *
 * File:   DeviceProxy.h
 */
#pragma once

#include "CharacteristicProxy.h"

#include <dbus-cxx.h>

#include <map>
#include <memory>
#include <optional>
#include <string>

#include "Proxy.h"
#include "DeviceMonitor.h"
#include "Thread/Mutex.hpp"

class DeviceMonitor;
class CharacteristicProxy;

/**
 * @brief Proxy class for a Bluetooth device using the BlueZ D-Bus API.
 *
 * The DeviceProxy class provides methods to interact with a Bluetooth device,
 * including connecting, disconnecting, discovering services, and accessing
 * characteristics. It also provides access to a DeviceMonitor for subscribing
 * to property changes.
 */
class DeviceProxy : public Proxy {
 public:
  using object_path = DBus::Path;
  using managed_objects = bluez::managed_objects;

  struct cache_entry {
    object_path path;
    uint16_t mtu;
  };

  DeviceProxy(std::shared_ptr<DBus::Connection> connection, object_path path);

  ~DeviceProxy();

  DeviceProxy() = delete;
  DeviceProxy(const DeviceProxy&) = delete;
  DeviceProxy& operator=(const DeviceProxy&) = delete;


  void Connect();

  void Disconnect();

  [[nodiscard]]
  bool IsConnected() const;

  [[nodiscard]]
  bool ServicesResolved() const;

  [[nodiscard]]
  std::optional<uint32_t> Class() const;

  [[nodiscard]]
  std::optional<std::string> Name() const;

  void DiscoverServices();


  [[nodiscard]]
  cache_entry FindCharacteristic(const uuid_t& service,
                                 const uuid_t& characteristic);

  [[nodiscard]]
  std::shared_ptr<CharacteristicProxy> Characteristic(
      const uuid_t& service, const uuid_t& characteristic);

  [[nodiscard]]
  const object_path& Path() const noexcept {
    return device_path_;
  }

  [[nodiscard]]
  DeviceMonitor& Monitor() noexcept {
    return *monitor_;
  }

  template <typename Handler>
  [[nodiscard]]
  Subscription SubscribeDiscoveredCharacteristics(Handler&& handler) {
    return discovered_characteristics_.Subscribe(std::forward<Handler>(handler));
  }

  [[nodiscard]]
  Subscription SubscribeProperty(std::string key, Event<DBus::Variant>::Handler handler) {
    return monitor_->SubscribeProperty(key, std::move(handler));
  }

 private:
  managed_objects GetManagedObjects() const;

  [[nodiscard]]
  std::shared_ptr<CharacteristicProxy> GetCharacteristicProxy(const object_path& path, uint16_t mtu);

  template <typename Signature>
  [[nodiscard]]
  std::shared_ptr<DBus::MethodProxy<Signature>>
  CreateMethod(const std::string& method_name) {
    return Proxy::CreateMethod<Signature>(bluez::kDeviceInterface, method_name);
  }

  template <typename TypeT>
  [[nodiscard]]
  std::shared_ptr<DBus::PropertyProxy<TypeT>>
  CreateProperty(const std::string& property_name) const {
    return Proxy::CreateProperty<TypeT>(bluez::kDeviceInterface, property_name);
  }

  
 private:

  object_path device_path_;

  std::shared_ptr<DeviceMonitor> monitor_;

  using characteristic_key = std::pair<uuid_t, uuid_t>;
  using characteristic_cache = std::map<characteristic_key, cache_entry>;

  Mutex characteristic_cache_mutex_;
  characteristic_cache characteristic_cache_;

  Mutex characteristic_proxies_mutex_;
  std::map<object_path, std::shared_ptr<CharacteristicProxy>> characteristic_proxies_;
  
  Event<uuid_t, uuid_t, std::vector<std::string>> discovered_characteristics_;
};