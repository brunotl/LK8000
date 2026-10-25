/*
 * LK8000 Tactical Flight Computer -  WWW.LK8000.IT
 * Released under GNU/GPL License v.2 or later
 * See CREDITS.TXT file for authors and copyrights
 *
 * File:   RFCommPort.h
 */
#pragma once

#include "Comm/SocketPort.h"
class RFCommPort : public SocketPort {
public:
    RFCommPort(int idx, const tstring& sName) : SocketPort(idx, sName) { }
    
protected:
    bool Connect() override;
};
