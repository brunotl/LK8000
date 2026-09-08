/*
 * LK8000 Tactical Flight Computer -  WWW.LK8000.IT
 * Released under GNU/GPL License v.2 or later
 * See CREDITS.TXT file for authors and copyrights
 *
 * File:   BlueZLeScanner.h
 *
 * TARGET=LINUX / TARGET=KOBO BLE device scanner, using BlueZ (via gattlib).
 * Same public interface as Android/BluetoothLeScanner so dlgConfiguration.cpp
 * uses identical code on both platforms.
 */

#ifndef COMM_BLUETOOTH_BLUEZLESCANNER_H
#define COMM_BLUETOOTH_BLUEZLESCANNER_H

#include "Comm/Bluetooth/GattlibBackend.h"

#include <functional>

class WndForm;

class BluetoothLeScanner {

  using callback_t = std::function<void(WndForm*, const char*, const char*, bool)>;

 public:
  BluetoothLeScanner() = delete;

  BluetoothLeScanner(WndForm* pWndForm, callback_t callback);
  ~BluetoothLeScanner();

 private:
  static void OnDiscovered(void* user_data, const char* address, const char* name, bool is_classic_spp);

  WndForm* _pWndForm;
  callback_t _callback;
  gattlib_backend::ScanHandle* handle = nullptr;
};

#endif  // COMM_BLUETOOTH_BLUEZLESCANNER_H
