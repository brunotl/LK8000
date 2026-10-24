/*
 * LK8000 Tactical Flight Computer -  WWW.LK8000.IT
 * Released under GNU/GPL License v.2 or later
 * See CREDITS.TXT file for authors and copyrights
 *
 * File:   CharacteristicProxy.h
 */
#pragma once

#include "Proxy.h"
#include "Event.h"
#include "Subscription.h"
#include "PropertyMonitor.h"

#include <memory>
#include <span>
#include <vector>

/**
 * @brief Proxy class for a Bluetooth GATT characteristic using the BlueZ D-Bus API.
 *
 * The CharacteristicProxy class provides methods to interact with a Bluetooth GATT characteristic,
 * including reading and writing values, and starting and stopping notifications.
 */
class CharacteristicProxy : public Proxy {
 public:
  using object_path = DBus::Path;
  using Value = std::vector<uint8_t>;
  using property_map = bluez::property_map;

  CharacteristicProxy(std::shared_ptr<DBus::Connection> connection,
                      object_path path, uint16_t mtu);

  ~CharacteristicProxy() = default;

  CharacteristicProxy(const CharacteristicProxy&) = delete;
  CharacteristicProxy& operator=(const CharacteristicProxy&) = delete;

  [[nodiscard]]
  Value Read();

  void WriteValue(Value value);
  void WriteValue(std::span<const uint8_t> data) {
    WriteValue(std::vector<uint8_t>(data.begin(), data.end()));
  }

  void StartNotify();

  void StopNotify();

  [[nodiscard]]
  Subscription SubscribeValue(Event<Value>::Handler&& handler) {
    return ValueChanged.Subscribe(std::move(handler));
  }

  [[nodiscard]]
  uint16_t MTU() const noexcept {
    return mtu_;
  }

 protected:
  Event<Value> ValueChanged;

  template <typename Signature>
  std::shared_ptr<DBus::MethodProxy<Signature>> CreateMethod(const std::string& method_name) {
    return Proxy::CreateMethod<Signature>(bluez::kGattCharacteristicInterface, method_name);
  }

 private:
  void OnValueChanged(const DBus::Variant& value);

  using write_proto = void(const std::vector<uint8_t>&, const property_map&);
  using read_proto = std::vector<uint8_t>(const property_map&);

  using start_notify_proto = void();
  using stop_notify_proto = void();

  using write_method = DBus::MethodProxy<write_proto>;
  using read_method = DBus::MethodProxy<read_proto>;
  
  using stop_notify_method = DBus::MethodProxy<stop_notify_proto>;
  using start_notify_method = DBus::MethodProxy<start_notify_proto>;

  std::shared_ptr<write_method> write;
  std::shared_ptr<read_method> read;

  std::shared_ptr<start_notify_method> start_notify;
  std::shared_ptr<stop_notify_method> stop_notify;

  Subscription property_monitor_subscription_;

  std::shared_ptr<PropertyMonitor> property_monitor_;
  uint16_t mtu_ = 23;
};
