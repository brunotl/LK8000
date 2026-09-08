/*
 * LK8000 Tactical Flight Computer -  WWW.LK8000.IT
 * Released under GNU/GPL License v.2 or later
 * See CREDITS.TXT file for authors and copyrights
 *
 * File:   BlueZLeScanner.cpp
 */

#include "BlueZLeScanner.h"

#include <stdexcept>
#include <utility>

BluetoothLeScanner::BluetoothLeScanner(WndForm* pWndForm, callback_t callback)
    : _pWndForm(pWndForm), _callback(std::move(callback)) {
  handle = gattlib_backend::StartScan(&BluetoothLeScanner::OnDiscovered, this);
  if (!handle) {
    throw std::runtime_error("Failed to start Bluetooth LE scan");
  }
}

BluetoothLeScanner::~BluetoothLeScanner() {
  gattlib_backend::StopScan(handle);
}

void BluetoothLeScanner::OnDiscovered(void* user_data, const char* address, const char* name, bool is_classic_spp) {
  auto* self = static_cast<BluetoothLeScanner*>(user_data);
  self->_callback(self->_pWndForm, address, name, is_classic_spp);
}
