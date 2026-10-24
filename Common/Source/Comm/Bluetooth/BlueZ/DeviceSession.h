/*
 * LK8000 Tactical Flight Computer -  WWW.LK8000.IT
 * Released under GNU/GPL License v.2 or later
 * See CREDITS.TXT file for authors and copyrights
 *
 * File:   DeviceSession.h
 */
#pragma once

#include "Subscription.h"
#include "Event.h"
#include "utils/uuid.h"

#include <atomic>
#include <chrono>
#include <memory>
#include <string>

class DeviceProxy;
class DeviceMonitor;
class CharacteristicProxy;

enum class DeviceState
{
    Disconnected,
    Connecting,
    Connected,
    Reconnecting,
    Unavailable
};

/**
 * @brief Represents a session for a Bluetooth device.
 *
 * The DeviceSession class manages the connection state, desired connection state,
 * and events related to a Bluetooth device. It provides methods to connect,
 * disconnect, and interact with GATT characteristics through the associated
 * DeviceProxy. It also allows subscribing to various events such as connection,
 * disconnection, state changes, and discovered characteristics.
 */
class DeviceSession : public std::enable_shared_from_this<DeviceSession> {
 public:
  explicit DeviceSession(std::string address);

  ~DeviceSession();

  DeviceSession(const DeviceSession&) = delete;
  DeviceSession& operator=(const DeviceSession&) = delete;

  void Connect();
  void Disconnect();

  [[nodiscard]]
  bool DesiredConnected() const noexcept {
    return desired_connected_;
  }

  [[nodiscard]]
  DeviceState State() const noexcept {
    return state_.load(std::memory_order_acquire);
  }

  [[nodiscard]]
  bool IsConnected() const noexcept {
    return State() == DeviceState::Connected;
  }

  [[nodiscard]]
  const std::string& Address() const noexcept {
    return address_;
  }

  std::shared_ptr<CharacteristicProxy> Characteristic(
      const uuid_t& service, const uuid_t& characteristic);

  [[nodiscard]]
  Subscription SubscribeConnected(Event<>::Handler&& handler) {
    return Connected.Subscribe(std::move(handler));
  }

  [[nodiscard]]
  Subscription SubscribeDisconnected(Event<>::Handler&& handler) {
    return Disconnected.Subscribe(std::move(handler));
  }

  [[nodiscard]]
  Subscription SubscribeStateChanged(Event<DeviceState>::Handler&& handler) {
    return StateChanged.Subscribe(std::move(handler));
  }

  [[nodiscard]]
  Subscription SubscribeDiscoveredCharacteristics(Event<uuid_t, uuid_t, std::vector<std::string>>::Handler&& handler) {
    return DiscoveredCharacteristics.Subscribe(std::move(handler));
  }

protected:
  Event<> Connected;
  Event<> Disconnected;

  Event<DeviceState> StateChanged;

  Event<uuid_t, uuid_t, std::vector<std::string>> DiscoveredCharacteristics;

 private:
  friend class BleManager;
  friend class DeviceMonitor;
  friend class DeviceProxy;

  // Called only by BleManager
  void TransitionTo(DeviceState state);

  void OnConnected();

  void OnDisconnected();

  void ResetBackoff();

  void IncreaseBackoff();

  void MarkAvailable();

  void MarkUnavailable();

  void AttachProxy(std::shared_ptr<DeviceProxy> proxy);

  void DetachProxy();

  [[nodiscard]]
  std::chrono::milliseconds CurrentBackoff() const noexcept {
    return reconnect_delay_;
  }

  // Called when a new GATT characteristic is discovered
  void OnDiscoveredCharacteristic(uuid_t service, uuid_t characteristic,
                                  std::vector<std::string> flags);

 private:
  std::string address_;

  // current state of the device session
  std::atomic<DeviceState> state_ = DeviceState::Disconnected;

  // whether the user intends the device to be connected
  std::atomic<bool> desired_connected_ = false;

  // current reconnect delay before attempting to reconnect
  std::chrono::milliseconds reconnect_delay_ = std::chrono::seconds(1); 

  // BlueZ side
  Subscription connected_sub_;
  Subscription services_resolved_sub_;
  Subscription discovered_characteristics_sub_;

  std::atomic<std::shared_ptr<DeviceProxy>> proxy_;

  [[nodiscard]]
  std::shared_ptr<DeviceProxy> CurrentProxy() const {
    return proxy_.load(std::memory_order_acquire);
  }

  bool reconnect_scheduled_ = false;  // worker only
};
