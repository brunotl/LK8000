/*
 * LK8000 Tactical Flight Computer -  WWW.LK8000.IT
 * Released under GNU/GPL License v.2 or later
 * See CREDITS.TXT file for authors and copyrights
 *
 * File:   CharacteristicProxy.cpp
 */
#include "CharacteristicProxy.h"
#include <cstdint>
#include <string_view>
#include "Common.h"
#include "MessageLog.h"

#define Log(...) BlueZDebugLog(_T("[CharacteristicProxy]") __VA_ARGS__)

using namespace bluez;

CharacteristicProxy::CharacteristicProxy(
    std::shared_ptr<DBus::Connection> connection, object_path path, uint16_t mtu)
    : Proxy(connection, kBluezServiceName, path), mtu_(mtu) {

  property_monitor_ = std::make_shared<PropertyMonitor>(*this, kGattCharacteristicInterface);

  property_monitor_subscription_ = property_monitor_->SubscribeProperty(
      "Value"s, [this](const DBus::Variant& value) {
        OnValueChanged(value);
      });
}

CharacteristicProxy::Value CharacteristicProxy::Read() {
  if (!read) {
    read = CreateMethod<read_proto>("ReadValue");
  }
  return (*read)({});
}

void CharacteristicProxy::WriteValue(CharacteristicProxy::Value value) {
  if (!write) {
    write = CreateMethod<write_proto>("WriteValue");
  }
  if (write) {
    static const std::map<std::string, DBus::Variant> opts{
        {"type"s, DBus::Variant("request"s)}};

    (*write)(std::move(value), opts);
  }
}

void CharacteristicProxy::StartNotify() {
  if (!start_notify) {
    start_notify = CreateMethod<start_notify_proto>("StartNotify");
  }
  if (start_notify) {
    (*start_notify)();
  }
}

void CharacteristicProxy::StopNotify() {
  if (!stop_notify) {
    stop_notify = CreateMethod<stop_notify_proto>("StopNotify");
  }
  if (stop_notify) {
    (*stop_notify)();
  }
}

void CharacteristicProxy::OnValueChanged(const DBus::Variant& value) {
  auto v = value;  // copy needed because to_vector is non-const
  ValueChanged.Notify(v.to_vector<uint8_t>());
}
