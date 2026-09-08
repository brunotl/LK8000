/*
 * LK8000 Tactical Flight Computer -  WWW.LK8000.IT
 * Released under GNU/GPL License v.2 or later
 * See CREDITS.TXT file for authors and copyrights
 *
 * File:   GattlibBackend.h
 *
 * A minimal, LK8000-agnostic wrapper around gattlib/BlueZ D-Bus GATT client
 * calls, used by BlueZGattSensor. Kept deliberately free of any LK8000
 * header (in particular utils/uuid.h): gattlib.h drags in BlueZ's
 * <bluetooth/bluetooth.h>, which declares its own unrelated `uuid_t`
 * (a legacy SDP struct) -- naming a plain 128-bit value the same as
 * LK8000's own uuid_t class. The two can't be visible in the same
 * translation unit, so this header/implementation pair is the wall between
 * them: BlueZGattSensor.cpp sees LK8000's uuid_t and this header only;
 * GattlibBackend.cpp sees gattlib.h and this header only, and converts
 * to/from the plain byte array below at the boundary.
 */

#ifndef COMM_BLUETOOTH_GATTLIBBACKEND_H
#define COMM_BLUETOOTH_GATTLIBBACKEND_H

#include <array>
#include <cstddef>
#include <cstdint>

namespace gattlib_backend {

/** Big-endian 128-bit UUID bytes, e.g. {00 00 FF E1 00 00 10 00 80 00 00 80 5F 9B 34 FB}. */
using uuid128_t = std::array<uint8_t, 16>;

struct Connection;

struct Callbacks {
  void* user_data = nullptr;

  /** The initial connection attempt (including service discovery) either
   *  succeeded or failed. */
  void (*on_connected)(void* user_data, bool success) = nullptr;

  /** The connection dropped after having been ready. A reconnect attempt
   *  is made internally; on_connected() is called again once it resolves. */
  void (*on_disconnected)(void* user_data) = nullptr;

  /** Asked once per discovered characteristic, to decide whether to
   *  subscribe to notifications (or, lacking that, do a one-shot read). */
  bool (*should_enable_notification)(void* user_data, const uuid128_t& service,
                                     const uuid128_t& characteristic) = nullptr;

  /** Delivers a notification, or the result of a read requested via
   *  ReadCharacteristic() (which echoes back the `service` it was given). */
  void (*on_characteristic_changed)(void* user_data, const uuid128_t& service,
                                    const uuid128_t& characteristic,
                                    const uint8_t* data, size_t length) = nullptr;
};

struct ScanHandle;

/**
 * Starts scanning for nearby BLE devices. `callback` is invoked (from an
 * internal thread, once per discovered or updated device) until StopScan()
 * is called; `name` may be empty if the device didn't advertise one.
 * `is_classic_spp` is true if BlueZ already knows this device offers the
 * classic Bluetooth Serial Port Profile (checked once per address, off the
 * calling thread) -- BlueZ's discovery isn't LE-only, so a scan commonly
 * surfaces classic-only devices (e.g. HC-05/HC-06-style UART bridges,
 * or a "SkyDrop SPP") alongside genuine BLE GATT ones; callers should use
 * this to offer a BT_SPP: port for those instead of a non-functional BLE:
 * one. Returns nullptr only on immediate/local failure (e.g. no Bluetooth
 * adapter available).
 */
ScanHandle* StartScan(void (*callback)(void* user_data, const char* address, const char* name,
                                       bool is_classic_spp),
                      void* user_data);

/**
 * Stops scanning and frees `handle`. Blocks until any in-flight callback
 * has returned.
 */
void StopScan(ScanHandle* handle);

/**
 * Starts an asynchronous BLE connection to `address` (e.g.
 * "AA:BB:CC:DD:EE:FF"). Returns nullptr only on immediate/local failure
 * (e.g. no Bluetooth adapter available); success or failure of the
 * connection itself is reported later through Callbacks::on_connected.
 *
 * If `has_write_characteristic`, `write_characteristic` is opened as a
 * chunked write stream once connected, for use by Write().
 */
Connection* Connect(const char* address, const uuid128_t& write_characteristic,
                    bool has_write_characteristic, const Callbacks& callbacks);

/**
 * Disconnects (if connected) and frees `connection`. Blocks until any
 * in-flight callback for this connection has returned.
 */
void Disconnect(Connection* connection);

/**
 * Writes to the characteristic configured at Connect() time, chunked to the
 * negotiated MTU. Returns false if not connected or no write characteristic
 * was configured.
 */
bool Write(Connection* connection, const void* data, size_t size);

/** Writes to an arbitrary characteristic (fire-and-forget, no chunking). */
bool WriteCharacteristic(Connection* connection, const uuid128_t& characteristic,
                         const void* data, size_t size);

/**
 * Reads an arbitrary characteristic asynchronously; the result (or nothing,
 * on failure) is delivered later through Callbacks::on_characteristic_changed,
 * echoing back `service` as given here.
 */
void ReadCharacteristic(Connection* connection, const uuid128_t& service,
                        const uuid128_t& characteristic);

} // namespace gattlib_backend

#endif  // COMM_BLUETOOTH_GATTLIBBACKEND_H
