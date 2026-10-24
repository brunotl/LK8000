/*
 * LK8000 Tactical Flight Computer -  WWW.LK8000.IT
 * Released under GNU/GPL License v.2 or later
 * See CREDITS.TXT file for authors and copyrights
 *
 * File:   PropertyMonitor.h
 */
#pragma once
#include <memory>
#include <unordered_map>

#include "Common.h"
#include "Event.h"
#include "SignalConnection.h"

/**
 * @brief Monitors changes to D-Bus properties on a specific interface.
 *
 * The PropertyMonitor class allows subscribing to changes of D-Bus properties
 * for a given interface on a specific object proxy. It provides thread-safe
 * event handling for property changes.
 *
 * Example usage:
 *
 *   auto monitor = std::make_shared<PropertyMonitor>(proxy, "org.bluez.Device1");
 *   auto subscription = monitor->SubscribeProperty("Name",
 *       [](const DBus::Variant& value) {
 *         // Handle property change
 *       });
 */
class PropertyMonitor : public std::enable_shared_from_this<PropertyMonitor> {
 public:
  using property_map = bluez::property_map;

  using signal_callback = void(const std::string, property_map,
                               std::vector<std::string>);
  using signal_proxy = DBus::SignalProxy<signal_callback>;

  using event_type = Event<const DBus::Variant&>;

  PropertyMonitor(std::shared_ptr<DBus::ObjectProxy> proxy,
                  std::string interface_name);

  ~PropertyMonitor() = default;

  [[nodiscard]]
  Subscription SubscribeProperty(const std::string& property, event_type::Handler&& callback);

  const std::shared_ptr<DBus::ObjectProxy>& Proxy() const {
    return proxy_;

  }

 protected:
  void OnPropertiesChanged(const property_map& changed);

  std::string interface_name_;
  mutable Mutex mutex_; // Protects access to event_changed_
  std::unordered_map<std::string, event_type> event_changed_;

  std::shared_ptr<DBus::ObjectProxy> proxy_;

  SignalConnection<signal_callback> signal_connection_;
};
