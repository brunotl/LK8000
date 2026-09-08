/*
 * LK8000 Tactical Flight Computer -  WWW.LK8000.IT
 * Released under GNU/GPL License v.2 or later
 * See CREDITS.TXT file for authors and copyrights
 *
 * File:   BlueZGattSensor.cpp
 */

#include "externs.h"
#include "BlueZGattSensor.h"
#include "Comm/Bluetooth/gatt_utils.h"

#include <stdexcept>
#include <utility>

namespace {

// The HM-10 and compatible bluetooth modules' data characteristic, used as
// the default write target for GattSensor::WriteData() -- matches the
// Android backend (Android/BluetoothGattClientPort.java's
// RX_TX_CHARACTERISTIC_UUID).
constexpr uuid_t HM10_RX_TX_CHARACTERISTIC = bluetooth::gatt_uuid(0xFFE1);

gattlib_backend::uuid128_t ToBackendUuid(uuid_t uuid) {
  gattlib_backend::uuid128_t out{};
  const uint64_t msb = uuid.msb();
  const uint64_t lsb = uuid.lsb();
  for (int i = 0; i < 8; ++i) {
    out[i] = static_cast<uint8_t>(msb >> (8 * (7 - i)));
    out[8 + i] = static_cast<uint8_t>(lsb >> (8 * (7 - i)));
  }
  return out;
}

uuid_t FromBackendUuid(const gattlib_backend::uuid128_t& b) {
  uint64_t msb = 0, lsb = 0;
  for (int i = 0; i < 8; ++i) {
    msb = (msb << 8) | b[i];
    lsb = (lsb << 8) | b[8 + i];
  }
  return uuid_t(msb, lsb);
}

} // namespace

bool BlueZGattSensor::Connect() {
  gattlib_backend::Callbacks callbacks;
  callbacks.user_data = this;
  callbacks.on_connected = &BlueZGattSensor::OnConnected;
  callbacks.on_disconnected = &BlueZGattSensor::OnDisconnected;
  callbacks.should_enable_notification = &BlueZGattSensor::ShouldEnableNotification;
  callbacks.on_characteristic_changed = &BlueZGattSensor::OnCharacteristicChangedCb;

  gattlib_backend::Connection* new_connection = gattlib_backend::Connect(
      GetPortName(), ToBackendUuid(HM10_RX_TX_CHARACTERISTIC), true, callbacks);
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
  return connection && gattlib_backend::Write(connection, data, size);
}

void BlueZGattSensor::DoWriteGattCharacteristic(const uuid_t& service, const uuid_t& characteristic, const void* data, size_t size) const {
  // gattlib writes by characteristic UUID only; `service` can't be used to
  // disambiguate a UUID reused across services.
  if (connection) {
    gattlib_backend::WriteCharacteristic(connection, ToBackendUuid(characteristic), data, size);
  }
}

void BlueZGattSensor::DoReadGattCharacteristic(const uuid_t& service, const uuid_t& characteristic) {
  if (connection) {
    gattlib_backend::ReadCharacteristic(connection, ToBackendUuid(service), ToBackendUuid(characteristic));
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

bool BlueZGattSensor::ShouldEnableNotification(void* user_data, const gattlib_backend::uuid128_t& service,
                                               const gattlib_backend::uuid128_t& characteristic) {
  auto* self = static_cast<BlueZGattSensor*>(user_data);
  return self->DoEnableNotification(FromBackendUuid(service), FromBackendUuid(characteristic));
}

void BlueZGattSensor::OnCharacteristicChangedCb(void* user_data, const gattlib_backend::uuid128_t& service,
                                                const gattlib_backend::uuid128_t& characteristic,
                                                const uint8_t* data, size_t length) {
  auto* self = static_cast<BlueZGattSensor*>(user_data);
  self->OnCharacteristicChanged(FromBackendUuid(service), FromBackendUuid(characteristic),
                                std::vector<uint8_t>(data, data + length));
}
