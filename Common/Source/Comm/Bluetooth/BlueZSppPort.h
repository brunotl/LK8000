/*
 * LK8000 Tactical Flight Computer -  WWW.LK8000.IT
 * Released under GNU/GPL License v.2 or later
 * See CREDITS.TXT file for authors and copyrights
 *
 * File:   BlueZSppPort.h
 *
 * TARGET=LINUX / TARGET=KOBO classic Bluetooth (BT_SPP:) port. Unlike BLE
 * GATT, classic RFCOMM needs no D-Bus/bluetoothd interaction from LK8000
 * itself: this connects a raw AF_BLUETOOTH/BTPROTO_RFCOMM socket, then uses
 * the kernel's RFCOMMCREATEDEV ioctl to bind it to a /dev/rfcommN tty
 * device node, and reuses TTYPort for all the actual serial I/O -- an
 * RFCOMM device node behaves like a regular tty once bound.
 */

#ifndef COMM_BLUETOOTH_BLUEZSPPPORT_H
#define COMM_BLUETOOTH_BLUEZSPPPORT_H

#include "Comm/TTYPort.h"

class BlueZSppPort : public TTYPort {
 public:
  BlueZSppPort(unsigned idx, const tstring& address);

  bool Close() override;

 protected:
  tstring GetDevicePath() override;

 private:
  void ReleaseRfcommDev();

  int _rfcomm_dev_id = -1;
};

#endif  // COMM_BLUETOOTH_BLUEZSPPPORT_H
