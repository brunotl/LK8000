/*
 * LK8000 Tactical Flight Computer -  WWW.LK8000.IT
 * Released under GNU/GPL License v.2 or later
 * See CREDITS.TXT file for authors and copyrights
 *
 * File:   PropertyMonitor.cpp
 */
#include "PropertyMonitor.h"
#include <dbus-cxx.h>
#include <functional>
#include <memory>
#include <string>
#include <cassert>
#include "BleManager.h"

namespace {

#define Log(...) BlueZDebugLog(_T("[PropertyMonitor]") __VA_ARGS__)

using namespace std::string_literals;

const auto kPropertiesInterface = "org.freedesktop.DBus.Properties"s;
const auto kPropertiesChangedSignal = "PropertiesChanged"s;

}  // namespace

PropertyMonitor::PropertyMonitor(std::shared_ptr<DBus::ObjectProxy> proxy,
                                 std::string interface_name)
    : interface_name_(std::move(interface_name)), proxy_(std::move(proxy)) {}

Subscription PropertyMonitor::SubscribeProperty(
    const std::string& property, event_type::Handler&& callback) {
  //
  bool is_first = false;
  event_type* event = nullptr;
  {
    std::lock_guard lock(mutex_);
    is_first = event_changed_.empty();
    event = &event_changed_.try_emplace(property).first->second;
  }
  auto subscription = event->Subscribe(std::move(callback));

  if (is_first) {
    // create the signal connection only once when the first property is subscribed.

    signal_connection_ = SignalConnection<signal_callback>(
        proxy_, kPropertiesInterface, kPropertiesChangedSignal,
        [weak_this = weak_from_this(), interface = interface_name_](
            const auto& interface_name, const auto& changed, const auto&) {
          if (interface_name == interface) {
            if (auto self = weak_this.lock()) {
              self->OnPropertiesChanged(changed);
            }
          }
        });
  }

  return subscription;
}

void PropertyMonitor::OnPropertiesChanged(const property_map& changed) {
  for (const auto& [key, value] : changed) {
    event_type* event = nullptr;
    {
      std::lock_guard lock(mutex_);
      auto it = event_changed_.find(key);
      if (it == event_changed_.end()) {
        continue;
      }
      event = &it->second;
    }
    if (event) {
      (*event)(value);
    }
  }
}
