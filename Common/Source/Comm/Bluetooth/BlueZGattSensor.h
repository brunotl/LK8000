/*
 * LK8000 Tactical Flight Computer -  WWW.LK8000.IT
 * Released under GNU/GPL License v.2 or later
 * See CREDITS.TXT file for authors and copyrights
 *
 * File:   BlueZGattSensor.h
 *
 * TARGET=LINUX / TARGET=KOBO Bluetooth LE GATT sensor, using BlueZ (via
 * gattlib) as the transport for GattSensor. Requires bluetoothd and a
 * working, powered hci adapter to already be present -- this class does
 * not bring up Bluetooth hardware itself.
 */

#ifndef COMM_BLUETOOTH_BLUEZGATTSENSOR_H
#define COMM_BLUETOOTH_BLUEZGATTSENSOR_H

#include "Comm/Bluetooth/GattSensor.h"
#include "Comm/Bluetooth/GattlibBackend.h"

#include <atomic>

class BlueZGattSensor : public GattSensor {
 public:
  using GattSensor::GattSensor;

 protected:
  bool Connect() override;
  void Disconnect() override;
  PortState GetPortState() const override;
  bool WriteData(const void* data, size_t size) override;
  void DoWriteGattCharacteristic(const uuid_t& service, const uuid_t& characteristic, const void* data, size_t size) const override;
  void DoReadGattCharacteristic(const uuid_t& service, const uuid_t& characteristic) override;

 private:
  static void OnConnected(void* user_data, bool success);
  static void OnDisconnected(void* user_data);
  static bool ShouldEnableNotification(void* user_data, const gattlib_backend::uuid128_t& service,
                                       const gattlib_backend::uuid128_t& characteristic);
  static void OnCharacteristicChangedCb(void* user_data, const gattlib_backend::uuid128_t& service,
                                        const gattlib_backend::uuid128_t& characteristic,
                                        const uint8_t* data, size_t length);

  mutable Mutex mutex;
  gattlib_backend::Connection* connection = nullptr;

  std::atomic<PortState> port_state{PortState::LIMBO};
};

#endif  // COMM_BLUETOOTH_BLUEZGATTSENSOR_H
