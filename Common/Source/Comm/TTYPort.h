/*
 * LK8000 Tactical Flight Computer -  WWW.LK8000.IT
 * Released under GNU/GPL License v.2 or later
 * See CREDITS.TXT file for authors and copyrights
 *
 * File:   TTYPort.h
 * Author: Bruno de Lacheisserie
 *
 * Created on 11 août 2014, 10:42
 */

#ifndef TTYPORT_H
#define	TTYPORT_H
#ifdef __linux__
#include "ComPort.h"
#include <atomic>
#include <termios.h>

class TTYPort : public ComPort {
public:
    TTYPort(unsigned idx, const tstring& sName, unsigned dwSpeed, BitIndex_t BitSize);
    ~TTYPort() override;

    bool Initialize() override;
    bool Close() override;

    void Flush() override;
    void Purge() override;
    void CancelWaitEvent() override;

    bool IsReady() override;

    int SetRxTimeout(int) override;
    unsigned long SetBaudrate(unsigned long) override;
    unsigned long GetBaudrate() const override;

    size_t Read(void *szString, size_t size) override;

protected:
    unsigned RxThread() override;

    // Overridable hook for the device node path to open() -- lets a
    // subclass (Comm/Bluetooth/BlueZSppPort, for BT_SPP: classic Bluetooth
    // ports) establish the underlying connection (RFCOMM connect + bind to
    // /dev/rfcommN) lazily, right before opening it, rather than needing a
    // fixed path known at construction time like a real tty.
    virtual tstring GetDevicePath() { return GetPortName(); }

private:

    unsigned _dwPortSpeed;
    BitIndex_t _dwPortBit;

    int _tty = -1;
    struct termios _oldtio = {};
    int _Timeout = RXTIMEOUT;

    bool Write_Impl(const void *data, size_t size) override;
};
#endif
#endif	/* TTYPORT_H */
