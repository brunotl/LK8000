/*
 * LK8000 Tactical Flight Computer -  WWW.LK8000.IT
 * Released under GNU/GPL License v.2 or later
 * See CREDITS.TXT file for authors and copyrights
 *
 * File:   DataFieldTime.h
 */

#ifndef _FORM_DATAFIELD_TIME_H_
#define _FORM_DATAFIELD_TIME_H_

#include "DataFieldFloat.h"

class DataFieldTime : public DataFieldFloat {
 public:
  DataFieldTime(WndProperty& Owner, const char* EditFormat, const char* DisplayFormat, double Min, double Max,
                double Step, int Fine, DataAccessCallback_t&& OnDataAccess);

  const TCHAR* GetAsDisplayString() override;
};

#endif  // _FORM_DATAFIELD_TIME_H_
