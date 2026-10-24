/*
 * LK8000 Tactical Flight Computer -  WWW.LK8000.IT
 * Released under GNU/GPL License v.2 or later
 * See CREDITS.TXT file for authors and copyrights
 *
 * File:   AdapterProxy.h
 */
#pragma once

#include "Common.h"
#include "options.h"
#include "Proxy.h"

/**
 * @brief Proxy class for a Bluetooth adapter using the BlueZ D-Bus API.
 *
 * The AdapterProxy class provides methods to interact with a Bluetooth adapter,
 * including starting and stopping discovery and setting discovery filters.
 */
class AdapterProxy final : public Proxy {
 public:
  using property_map = bluez::property_map;

  using method_void = void();
  using method_property_map = void(const property_map&);

  using method_proxy_void = DBus::MethodProxy<method_void>;
  using method_proxy_property_map = DBus::MethodProxy<method_property_map>;

  explicit AdapterProxy(std::shared_ptr<DBus::ObjectProxy> object)
      : Proxy(std::move(object)),
        set_discovery_filter(CreateMethod<method_property_map>("SetDiscoveryFilter")),
        start_discovery(CreateMethod<method_void>("StartDiscovery")),
        stop_discovery(CreateMethod<method_void>("StopDiscovery")) {}

  AdapterProxy(std::shared_ptr<DBus::Connection> connection, const std::string& path)
      : AdapterProxy(CreateProxy(connection, bluez::kBluezServiceName, path)) {}

  void SetDiscoveryFilter(const property_map& filter) {
    (*set_discovery_filter)(filter);
  }

  void StartDiscovery() {
    (*start_discovery)();
  }

  void StopDiscovery() {
    (*stop_discovery)();
  }

 private:
  
  template <typename Signature>
  std::shared_ptr<DBus::MethodProxy<Signature>> CreateMethod(const std::string& method_name) {
    return Proxy::CreateMethod<Signature>(bluez::kAdapterInterface, method_name);
  }

  std::shared_ptr<method_proxy_property_map> set_discovery_filter;
  std::shared_ptr<method_proxy_void> start_discovery;
  std::shared_ptr<method_proxy_void> stop_discovery;
};
