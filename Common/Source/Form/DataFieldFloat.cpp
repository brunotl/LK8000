/*
 * LK8000 Tactical Flight Computer -  WWW.LK8000.IT
 * Released under GNU/GPL License v.2 or later
 * See CREDITS.TXT file for authors and copyrights
 *
 * File:   DataFieldFloat.cpp
 */

#include "DataFieldFloat.h"
#include "Util/Clamp.hpp"
#include "Dialogs.h"
#include "ComboList.h"
#include "MathFunctions.h"
#include "Utils.h"

DataFieldFloat::DataFieldFloat(WndProperty& Owner, const char* EditFormat, const char* DisplayFormat, double Min,
                               double Max, double Step, int Fine, DataAccessCallback_t&& OnDataAccess)
    : DataField(Owner, EditFormat, DisplayFormat, std::forward<DataAccessCallback_t>(OnDataAccess)),
      mMin(Min), mMax(Max), mStep(Step), mFine(Fine)
{
  SupportCombo = true;
  mOnDataAccess(this, daGet);
}

void DataFieldFloat::Set(double Value) {
  mValue = Clamp(Value, mMin, mMax);
}

const TCHAR* DataFieldFloat::GetAsDisplayString() {
  _stprintf(mOutBuf, mDisplayFormat, mValue, mUnits);
  return mOutBuf;
}

double DataFieldFloat::SetMin(double Value) {
  double res = mMin;
  mMin = Value;
  return res;
}

double DataFieldFloat::SetMax(double Value) {
  double res = mMax;
  mMax = Value;
  return res;
}

double DataFieldFloat::SetStep(double Value) {
  double res = mStep;
  mStep = Value;
  return res;
}

bool DataFieldFloat::GetAsBoolean() {
  return mValue != 0.0;
}

int DataFieldFloat::GetAsInteger() {
  return iround(mValue);
}

double DataFieldFloat::GetAsFloat() {
  return mValue;
}

const TCHAR* DataFieldFloat::GetAsString() {
  _stprintf(mOutBuf, mEditFormat, mValue);
  return mOutBuf;
}

bool DataFieldFloat::SetAsBoolean(bool Value) {
  bool res = GetAsBoolean();
  if (res != Value) {
    if (Value)
      SetAsFloat(1.0);
    else
      SetAsFloat(0.0);
  }
  return res;
}

int DataFieldFloat::SetAsInteger(int Value) {
  int res = GetAsInteger();
  SetAsFloat(Value);
  return res;
}

double DataFieldFloat::SetAsFloat(double Value) {
  double res = mValue;
  Value = Clamp(Value, mMin, mMax);
  if (res != Value) {
    mValue = Value;
    if (!GetDetachGUI()) {
      mOnDataAccess(this, daChange);
    }
  }
  return res;
}

const TCHAR* DataFieldFloat::SetAsString(const TCHAR* Value) {
  const TCHAR* res = GetAsString();
  SetAsFloat(_tcstod(Value, NULL));
  return res;
}

void DataFieldFloat::Inc() {
  if (mFine && (mValue < 0.95) && (mStep >= 0.5) && (mMin >= 0.0)) {
    SetAsFloat(mValue + 0.1);
  } else {
    SetAsFloat(mValue + mStep * SpeedUp(true));
  }
}

void DataFieldFloat::Dec() {
  if (mFine && (mValue <= 1.0) && (mStep >= 0.5) && (mMin >= 0.0)) {
    SetAsFloat(mValue - 0.1);
  } else {
    SetAsFloat(mValue - mStep * SpeedUp(false));
  }
}

double DataFieldFloat::SpeedUp(bool keyup) {
  double res = 1.0;
  if (GetDisableSpeedUp()) {
    return 1;
  }
  if (keyup != DataFieldKeyUp) {
    mSpeedup = 0;
    DataFieldKeyUp = keyup;
    mTmLastStep.Update();
    return 1.0;
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

int DataFieldFloat::SetFromCombo(int iDataFieldIndex, const TCHAR* sValue) {
  SetAsString(sValue);
  return 0;
}

bool DataFieldFloat::CreateKeyboard() {
  TCHAR szText[20];
  _tcscpy(szText, GetAsString());
  dlgNumEntryShowModal(szText, 20);
  SetAsFloat(floor((StrToDouble(szText, nullptr) / mStep) + 0.5) * mStep);
  return true;
}

int DataFieldFloat::CreateComboList() {
  return CreateComboListStepping();
}
