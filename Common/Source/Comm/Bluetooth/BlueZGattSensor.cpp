/*
 * LK8000 Tactical Flight Computer -  WWW.LK8000.IT
 * Released under GNU/GPL License v.2 or later
 * See CREDITS.TXT file for authors and copyrights
 *
 * File:   BlueZGattSensor.cpp
 */

#include "externs.h"
#include "BlueZGattSensor.h"
#include "gatt_utils.h"

#include <stdexcept>
#include <utility>

bool BlueZGattSensor::Connect() {
  gattlib_backend::Callbacks callbacks = {
    .user_data = this,
    .on_connected = &BlueZGattSensor::OnConnected,
    .on_disconnected = &BlueZGattSensor::OnDisconnected,
    .should_enable_notification = &BlueZGattSensor::ShouldEnableNotification,
    .on_characteristic_changed = &BlueZGattSensor::OnCharacteristicChangedCb,
  };

  gattlib_backend::Connection* new_connection =
      gattlib_backend::Connect(GetPortName(), callbacks);

  if (!new_connection) {
    throw std::runtime_error("Failed to start Bluetooth LE connection");
  }

  const std::lock_guard lock(mutex);
  connection = new_connection;
  return true;
}

void BlueZGattSensor::Disconnect() {
  gattlib_backend::Connection* old_connection = WithLock(mutex, [&]() {
    return std::exchange(connection, nullptr);
  });
  gattlib_backend::Disconnect(old_connection);
}

GattSensor::PortState BlueZGattSensor::GetPortState() const {
  return port_state.load();
}

bool BlueZGattSensor::WriteData(const void* data, size_t size) {
  const std::lock_guard lock(mutex);
  return connection && gattlib_backend::Write(connection, data, size);
}

void BlueZGattSensor::DoWriteGattCharacteristic(const uuid_t& service, const uuid_t& characteristic, const void* data, size_t size) const {
  const std::lock_guard lock(mutex);
  // gattlib writes by characteristic UUID only; `service` can't be used to
  // disambiguate a UUID reused across services.
  if (connection) {
    gattlib_backend::WriteCharacteristic(connection, characteristic, data, size);
  }
}

void BlueZGattSensor::DoReadGattCharacteristic(const uuid_t& service, const uuid_t& characteristic) {
  const std::lock_guard lock(mutex);
  if (connection) {
    gattlib_backend::ReadCharacteristic(connection, service, characteristic);
  }
}

void BlueZGattSensor::OnConnected(void* user_data, bool success) {
  auto* self = static_cast<BlueZGattSensor*>(user_data);
  self->port_state = success ? PortState::READY : PortState::FAILED;
  if (!success) {
    self->PortError("Failed to connect to Bluetooth LE device");
  }
  self->PortStateChanged();
}

void BlueZGattSensor::OnDisconnected(void* user_data) {
  auto* self = static_cast<BlueZGattSensor*>(user_data);
  self->port_state = PortState::LIMBO;
  self->PortStateChanged();
}

bool BlueZGattSensor::ShouldEnableNotification(void* user_data, const uuid_t& service,
                                               const uuid_t& characteristic) {
  auto* self = static_cast<BlueZGattSensor*>(user_data);
  return self->DoEnableNotification(service, characteristic);
}

void BlueZGattSensor::OnCharacteristicChangedCb(void* user_data, const uuid_t& service,
                                                const uuid_t& characteristic,
                                                const uint8_t* data, size_t length) {
  auto* self = static_cast<BlueZGattSensor*>(user_data);
  self->OnCharacteristicChanged(service, characteristic,
                                std::vector<uint8_t>(data, data + length));
}
