/*
 * LK8000 Tactical Flight Computer -  WWW.LK8000.IT
 * Released under GNU/GPL License v.2 or later
 * See CREDITS.TXT file for authors and copyrights
 *
 * File:   BlueZGattSensor.cpp
 */
#include "BlueZGattSensor.h"

#include <algorithm>
#include <exception>
#include <map>
#include <stdexcept>
#include <string>
#include <utility>
#include <vector>
#include "MessageLog.h"
#include "BleManager.h"
#include "../gatt_utils.h"

#define Log(...) BlueZDebugLog(_T("[BlueZGattSensor]") __VA_ARGS__)

BlueZGattSensor::~BlueZGattSensor() {
  try {
    const std::lock_guard lock(mutex_);
    if (session_) {
      session_->Disconnect();
    }
  }
  catch (const std::exception& e) {
    Log("Disconnect exception: %s", e.what());
  }
}

bool BlueZGattSensor::Connect() {
  auto session = BleManager::Instance().Open(GetPortName());

  connected_sub_ = session->SubscribeConnected([this] {
    PortStateChanged();
  });

  disconnected_sub_ = session->SubscribeDisconnected([this] {
    PortStateChanged();
  });

  discovered_characteristics_sub_ =
      session->SubscribeDiscoveredCharacteristics(
          [this](uuid_t service, uuid_t characteristic,
                 std::vector<std::string> flags) {
            OnCharacteristicDiscovered(service, characteristic, flags);
          });

  {
    const std::lock_guard lock(mutex_);
    session_ = session;
  }
  session->Connect();

  return true;
}

void BlueZGattSensor::Disconnect() {

  std::shared_ptr<DeviceSession> session_to_close;
  {
    const std::lock_guard lock(mutex_);
    value_changed_sub_.clear();
    session_to_close = std::exchange(session_, nullptr);
  }
  discovered_characteristics_sub_ = {};
  connected_sub_ = {};
  disconnected_sub_ = {};

  if (session_to_close) {
    session_to_close->Disconnect();
    session_to_close.reset();
  }
}

std::shared_ptr<DeviceSession> BlueZGattSensor::GetSession() const {
  const std::lock_guard lock(mutex_);
  if (!session_) {
    throw std::runtime_error("No active session");
  }
  return session_;
}

GattSensor::PortState BlueZGattSensor::GetPortState() const {
  const std::lock_guard lock(mutex_);
  if (session_ && session_->IsConnected()) {
    return PortState::READY;
  }
  return PortState::LIMBO;
}

void BlueZGattSensor::WriteGattCharacteristic(const uuid_t& service,
                                              const uuid_t& characteristic,
                                              const void* data,
                                              size_t size) const {

  try {
    std::shared_ptr<DeviceSession> session = GetSession();

    auto characteristic_proxy =
        session->Characteristic(service, characteristic);
    auto udata = static_cast<const uint8_t*>(data);
    characteristic_proxy->WriteValue(std::span(udata, size));
  }
  catch (const std::exception& e) {
    Log(_T("WriteGattCharacteristic exception: %s"), e.what());
  }
}

void BlueZGattSensor::ReadGattCharacteristic(const uuid_t& service,
                                             const uuid_t& characteristic) {
  Log(_T("Reading characteristic: %s"), characteristic.to_string().c_str());
  try {
    std::shared_ptr<DeviceSession> session = GetSession();

    auto characteristic_proxy =
        session->Characteristic(service, characteristic);
    auto data = characteristic_proxy->Read();
    OnCharacteristicChanged(service, characteristic, std::move(data));
  }
  catch (const std::exception& e) {
    // Handle the exception, e.g., log it
    Log(_T("ReadGattCharacteristic exception: %s"), e.what());
  }
}

bool BlueZGattSensor::WriteData(const void* data, size_t size) {
  try {
    std::shared_ptr<DeviceSession> session = GetSession();

    constexpr auto HM10_SERVICE =  bluetooth::gatt_uuid(0xFFE0);
    constexpr auto RX_TX_CHARACTERISTIC_UUID = bluetooth::gatt_uuid(0xFFE1);

    // Implement the actual data writing logic here using the session
    auto characteristic_proxy =
        session->Characteristic(HM10_SERVICE, RX_TX_CHARACTERISTIC_UUID);
    auto udata = static_cast<const uint8_t*>(data);

    const size_t mtu = characteristic_proxy->MTU();
    const size_t chunk = mtu - 3;

    for (size_t off = 0; off < size; off += chunk) {
      size_t len = std::min(chunk, size - off);

      characteristic_proxy->WriteValue(std::span(udata + off, len));

      // No direct error handling for characteristic_proxy->Write
      // as it may throw an exception which will be caught by the outer try-catch block.
    }
    return true;
  }
  catch (const std::exception& e) {
    Log(_T("WriteData exception: %s"), e.what());
    return false;
  }
}

void BlueZGattSensor::SubscribeCharacteristic(const uuid_t& service,
                                              const uuid_t& characteristic) {
  Log(_T("Subscribing to characteristic: %s"), characteristic.to_string().c_str());
  try {
    std::shared_ptr<DeviceSession> session = GetSession();

    auto characteristic_proxy =
        session->Characteristic(service, characteristic);

    auto subscription = characteristic_proxy->SubscribeValue(
        [this, service, characteristic](std::vector<uint8_t> value) {
          OnCharacteristicChanged(service, characteristic, std::move(value));
        });

    {
      const std::lock_guard lock(mutex_);
      value_changed_sub_.push_back(std::move(subscription));
    }

    characteristic_proxy->StartNotify();
  }
  catch (const std::exception& e) {
    Log(_T("SubscribeCharacteristic exception: %s"), e.what());
  }
}

void BlueZGattSensor::OnCharacteristicDiscovered(
    uuid_t service, uuid_t characteristic, std::vector<std::string> flags) {
  if (DoEnableNotification(service, characteristic)) {
    if (bluez::CheckFlags(flags, "notify", "indicate")) {
      SubscribeCharacteristic(service, characteristic);
    }
    if (bluez::CheckFlags(flags, "read")) {
      ReadGattCharacteristic(service, characteristic);
    }
  }
}
