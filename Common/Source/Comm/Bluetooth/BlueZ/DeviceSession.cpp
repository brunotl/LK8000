/*
 * LK8000 Tactical Flight Computer -  WWW.LK8000.IT
 * Released under GNU/GPL License v.2 or later
 * See CREDITS.TXT file for authors and copyrights
 *
 * File:   DeviceSession.cpp
 */
#include "DeviceSession.h"
#include <memory>
#include "DeviceMonitor.h"
#include "DeviceProxy.h"
#include "BleManager.h"
#include "MessageLog.h"

#define Log(...) BlueZDebugLog(_T("[DeviceSession]") __VA_ARGS__)

DeviceSession::DeviceSession(std::string address)
    : address_(std::move(address)) {}

DeviceSession::~DeviceSession() = default;

void DeviceSession::Connect() {
  desired_connected_.store(true, std::memory_order_release);

  BleManager::Instance().RequestConnect(shared_from_this());
}

void DeviceSession::Disconnect() {
  desired_connected_.store(false, std::memory_order_release);
  BleManager::Instance().RequestDisconnect(shared_from_this());
}

void DeviceSession::TransitionTo(DeviceState state) {
  auto previous = state_.exchange(state, std::memory_order_acq_rel);

  if (previous == state) {
    return;
  }

  StateChanged(state);
}

void DeviceSession::OnConnected() {
  Log("%s : Connected", address_.c_str());
  ResetBackoff();

  TransitionTo(DeviceState::Connected);
  Connected();
}

void DeviceSession::OnDisconnected() {
  Log("%s : Disconnected", address_.c_str());
  const bool was_connected = State() == DeviceState::Connected;

  TransitionTo(desired_connected_.load(std::memory_order_acquire)
                   ? DeviceState::Reconnecting
                   : DeviceState::Disconnected);

  if (was_connected) {
    Disconnected();
  }
}

void DeviceSession::OnDiscoveredCharacteristic(uuid_t service,
                                               uuid_t characteristic,
                                               std::vector<std::string> flags) {
  DiscoveredCharacteristics(service, characteristic, flags);
}

std::shared_ptr<CharacteristicProxy> DeviceSession::Characteristic(
    const uuid_t& service, const uuid_t& characteristic) {
  auto proxy = proxy_.load(std::memory_order_acquire);

  if (!proxy) {
    throw std::runtime_error("Device session has no active proxy: " + address_);
  }
  return proxy->Characteristic(service, characteristic);
}

void DeviceSession::ResetBackoff() {
  reconnect_delay_ = std::chrono::seconds(1);
}

void DeviceSession::IncreaseBackoff() {
  reconnect_delay_ = std::min<std::chrono::milliseconds>(
      reconnect_delay_ * 2, std::chrono::seconds(30));
}

void DeviceSession::MarkAvailable() {
  if (state_ == DeviceState::Unavailable) {
    TransitionTo(DeviceState::Disconnected);
  }
}

void DeviceSession::MarkUnavailable() {
  TransitionTo(DeviceState::Unavailable);
}

void DeviceSession::AttachProxy(std::shared_ptr<DeviceProxy> proxy) {
  auto current_proxy = proxy_.load(std::memory_order_acquire);
  if (current_proxy) {
    DetachProxy();
  }

  proxy_.store(proxy, std::memory_order_release);

  discovered_characteristics_sub_ = proxy->SubscribeDiscoveredCharacteristics(
      [this](uuid_t service, uuid_t characteristic,
             std::vector<std::string> flags) {
        OnDiscoveredCharacteristic(service, characteristic, flags);
      });

  connected_sub_ = proxy->SubscribeProperty(
      "Connected", [weak_self = weak_from_this(),
                    weak_proxy = std::weak_ptr(proxy)](bool connected) {
        BleManager::Instance().PostProxyConnectionChanged(weak_self, weak_proxy,
                                                          connected);
      });

  services_resolved_sub_ =
      proxy->SubscribeProperty("ServicesResolved", [proxy = std::weak_ptr(proxy)](bool resolved) {
        if (resolved) {
          auto& runtime = BleManager::Instance().Runtime();
          runtime.Post([proxy]() {
            if (auto p = proxy.lock()) {
              p->DiscoverServices();
            }
          });
        }
      });
}

void DeviceSession::DetachProxy() {
  connected_sub_.Reset();
  services_resolved_sub_.Reset();
  discovered_characteristics_sub_.Reset();
  proxy_.store(nullptr, std::memory_order_release);
}
