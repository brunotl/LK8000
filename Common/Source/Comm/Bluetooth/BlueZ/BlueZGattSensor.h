/*
 * LK8000 Tactical Flight Computer -  WWW.LK8000.IT
 * Released under GNU/GPL License v.2 or later
 * See CREDITS.TXT file for authors and copyrights
 *
 * File:   BlueZGattSensor.h
 */

#pragma once

#include "options.h"
#include "../GattSensor.h"
#include "Thread/Mutex.hpp"

#include "Common.h"
#include "CharacteristicProxy.h"
#include "DeviceSession.h"
#include "Subscription.h"


#include <memory>
#include <string>
#include <vector>


class BlueZGattSensor final : public GattSensor {
 public:
  using GattSensor::GattSensor;

  ~BlueZGattSensor() override;

 protected:
  bool Connect() override;

  void Disconnect() override;

  PortState GetPortState() const override;

  bool WriteData(const void* data, size_t size) override;

  void WriteGattCharacteristic(const uuid_t& service,
                               const uuid_t& characteristic, const void* data,
                               size_t size) const override;

  void ReadGattCharacteristic(const uuid_t& service,
                              const uuid_t& characteristic) override;

 private:
  void SubscribeCharacteristic(const uuid_t& service,
                               const uuid_t& characteristic);

  void OnCharacteristicDiscovered(uuid_t service, uuid_t characteristic,
                                  std::vector<std::string> flags);

  void OnConnected();
  void OnDisconnected();

  std::shared_ptr<DeviceSession> GetSession() const;
  
 private:
  mutable Mutex mutex_;

  std::shared_ptr<DeviceSession> session_;

  Subscription connected_sub_;
  Subscription disconnected_sub_;
  Subscription discovered_characteristics_sub_;
  std::list<Subscription> value_changed_sub_;
};
