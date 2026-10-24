/*
 * LK8000 Tactical Flight Computer -  WWW.LK8000.IT
 * Released under GNU/GPL License v.2 or later
 * See CREDITS.TXT file for authors and copyrights
 *
 * File:   BluetoothLeScanner.h
 * Author: Bruno de Lacheisserie
 */

#include "Compiler.h"
#include "Java/Object.hxx"
#include "Android/LeScanCallback.hpp"
#include <functional>

class WndForm;

class BluetoothLeScanner : public LeScanCallback {

  using callback_t = std::function<void(WndForm *, const char *, const char *, bool)>;

 public:
  static void Initialise(JNIEnv* env);
  static void Deinitialise(JNIEnv* env);

  BluetoothLeScanner() = delete;

  BluetoothLeScanner(WndForm *pWndForm, callback_t callback);
  ~BluetoothLeScanner() override;

 private:

  void OnLeScan(const char *address, const char *name) override {
    // Android's LE scan API never surfaces classic-only devices (unlike
    // BlueZ's combined discovery on Linux/Kobo), so this is always false.
    _callback(_pWndForm, address, name, false);
  }

  WndForm *_pWndForm;
  callback_t _callback;
  Java::LocalObject obj;
};
