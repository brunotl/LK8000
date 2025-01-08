/*
 * LK8000 Tactical Flight Computer -  WWW.LK8000.IT
 * Released under GNU/GPL License v.2 or later
 * See CREDITS.TXT file for authors and copyrights
 *
 * File:   DataFieldBoolean.cpp
 */

#include "DataFieldBoolean.h"

DataFieldBoolean::DataFieldBoolean(WndProperty& Owner, const char* EditFormat, const char* DisplayFormat, int Default,
                                   const TCHAR* TextTrue, const TCHAR* TextFalse, DataAccessCallback_t&& OnDataAccess)
    : DataField(Owner, EditFormat, DisplayFormat, std::move(OnDataAccess)) {
  if (Default) {
    mValue = true;
  } else {
    mValue = false;
  }
  _tcscpy(mTextTrue, TextTrue);
  _tcscpy(mTextFalse, TextFalse);
  SupportCombo = true;
  mOnDataAccess(this, daGet);
}

void DataFieldBoolean::Inc() {
  SetAsBoolean(!GetAsBoolean());
}

void DataFieldBoolean::Dec() {
  SetAsBoolean(!GetAsBoolean());
}

int DataFieldBoolean::CreateComboList() {
  mComboList.push_back(0, mTextFalse, mTextFalse);
  mComboList.push_back(1, mTextTrue, mTextTrue);
  mComboList.PropertyDataFieldIndexSaved = (GetAsInteger() < 2) ? GetAsInteger() : 0;
  return mComboList.size();
}

bool DataFieldBoolean::GetAsBoolean() {
  return mValue;
}

int DataFieldBoolean::GetAsInteger() {
  return mValue ? 1 : 0;
}

double DataFieldBoolean::GetAsFloat() {
  return mValue ? 1.0 : 0.0;
}

const TCHAR* DataFieldBoolean::GetAsString() {
  return mValue ? mTextTrue : mTextFalse;
}

void DataFieldBoolean::Set(bool Value) {
  mValue = Value;
}

void DataFieldBoolean::Set(int Value) {
  mValue = Value;
}

bool DataFieldBoolean::SetAsBoolean(bool Value) {
  bool res = mValue;
  if (mValue != Value) {
    mValue = Value;
    if (!GetDetachGUI()) {
      mOnDataAccess(this, daChange);
    }
  }
  return res;
}

int DataFieldBoolean::SetAsInteger(int Value) {
  int res = GetAsInteger();
  if (GetAsInteger() != Value) {
    SetAsBoolean(!(Value == 0));
  }
  return res;
}

double DataFieldBoolean::SetAsFloat(double Value) {
  double res = GetAsFloat();
  if (GetAsFloat() != Value) {
    SetAsBoolean(!(Value == 0.0));
  }
  return res;
}

const TCHAR* DataFieldBoolean::SetAsString(const TCHAR* Value) {
  const TCHAR* res = GetAsString();
  if (_tcscmp(res, Value) != 0) {
    SetAsBoolean(_tcscmp(Value, mTextTrue) == 0);
  }
  return res;
}
