/*
 * LK8000 Tactical Flight Computer -  WWW.LK8000.IT
 * Released under GNU/GPL License v.2 or later
 * See CREDITS.TXT file for authors and copyrights
 *
 * File:   DataFieldInteger.cpp
 */

#include "DataFieldInteger.h"
#include "Dialogs.h"
#include "Util/Clamp.hpp"
#include "externs.h"
#include "utils/stringext.h"

// DataFieldInteger class implementation
DataFieldInteger::DataFieldInteger(WndProperty& Owner, const char* EditFormat, const char* DisplayFormat, int Min,
                                   int Max, int Default, int Step, DataAccessCallback_t&& OnDataAccess)
    : DataField(Owner, EditFormat, DisplayFormat, std::move(OnDataAccess)),
      mMin(Min),
      mMax(Max),
      mValue(Default),
      mStep(Step),
      mSpeedup(0) {
  SupportCombo = true;
  mOnDataAccess(this, daGet);
}

void DataFieldInteger::Inc() {
  SetAsInteger(mValue + mStep * SpeedUp(true));
}

void DataFieldInteger::Dec() {
  SetAsInteger(mValue - mStep * SpeedUp(false));
}

int DataFieldInteger::CreateComboList() {
  return CreateComboListStepping();
}

bool DataFieldInteger::CreateKeyboard() {
  TCHAR szText[20];
  _tcscpy(szText, GetAsString());
  dlgNumEntryShowModal(szText, 20);
  SetAsFloat((int)floor((StrToDouble(szText, nullptr) / mStep) + 0.5) * mStep);
  return true;
}

bool DataFieldInteger::GetAsBoolean() {
  return mValue != 0;
}

int DataFieldInteger::GetAsInteger() {
  return mValue;
}

double DataFieldInteger::GetAsFloat() {
  return mValue;
}

const TCHAR* DataFieldInteger::GetAsString() {
  _stprintf(mOutBuf, mEditFormat, mValue);
  return mOutBuf;
}

const TCHAR* DataFieldInteger::GetAsDisplayString() {
  _stprintf(mOutBuf, mDisplayFormat, mValue, mUnits);
  return mOutBuf;
}

void DataFieldInteger::Set(int Value) {
  mValue = Value;
}

bool DataFieldInteger::SetAsBoolean(bool Value) {
  bool res = GetAsBoolean();
  if (Value) {
    SetAsInteger(1);
  } else {
    SetAsInteger(0);
  }
  return res;
}

int DataFieldInteger::SetAsInteger(int Value) {
  int res = mValue;
  if (Value < mMin) {
    Value = mMin;
  }
  if (Value > mMax) {
    Value = mMax;
  }
  if (mValue != Value) {
    mValue = Value;
    if (!GetDetachGUI()) {
      mOnDataAccess(this, daChange);
    }
  }
  return res;
}

double DataFieldInteger::SetAsFloat(double Value) {
  double res = GetAsFloat();
  SetAsInteger(iround(Value));
  return res;
}

const TCHAR* DataFieldInteger::SetAsString(const TCHAR* Value) {
  const TCHAR* res = GetAsString();
  SetAsInteger(_tcstol(Value, NULL, 10));
  return res;
}

int DataFieldInteger::SpeedUp(bool keyup) {
  int res = 1;
  if (GetDisableSpeedUp() == true) {
    return 1;
  }
  if (keyup != DataFieldKeyUp) {
    mSpeedup = 0;
    DataFieldKeyUp = keyup;
    mTmLastStep.Update();
    return 1;
  }
  if (!mTmLastStep.Check(200)) {
    mSpeedup++;
    if (mSpeedup > 5) {
      res = 10;
      mTmLastStep.UpdateWithOffset(350);  // now + 350ms
      return res;
    }
  } else {
    mSpeedup = 0;
  }
  mTmLastStep.Update();
  return res;
}
