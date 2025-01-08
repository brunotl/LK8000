/*
 * LK8000 Tactical Flight Computer -  WWW.LK8000.IT
 * Released under GNU/GPL License v.2 or later
 * See CREDITS.TXT file for authors and copyrights
 *
 * File:   DataFieldTime.cpp
 */

#include "DataFieldTime.h"
#include <cmath>

DataFieldTime::DataFieldTime(WndProperty& Owner, const char* EditFormat, const char* DisplayFormat, double Min,
                             double Max, double Default, double Step, int Fine, DataAccessCallback_t&& OnDataAccess)
    : DataFieldFloat(Owner, EditFormat, DisplayFormat, Min, Max, Default, Step, Fine, std::move(OnDataAccess)) {}

const TCHAR* DataFieldTime::GetAsDisplayString() {
  double hours = 0;
  double minutes = modf(mValue, &hours);
  _stprintf(mOutBuf, mDisplayFormat, (int)hours, (int)std::abs(minutes * 60.));
  return mOutBuf;
}
