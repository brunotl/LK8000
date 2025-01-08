/*
 * LK8000 Tactical Flight Computer -  WWW.LK8000.IT
 * Released under GNU/GPL License v.2 or later
 * See CREDITS.TXT file for authors and copyrights
 *
 * File:   DataFieldString.cpp
 */

#include "DataFieldString.h"
#include "Dialogs.h"

DataFieldString::DataFieldString(WndProperty& Owner, const char* EditFormat, const char* DisplayFormat,
                                 const TCHAR* Default, DataAccessCallback_t&& OnDataAccess)
    : DataField(Owner, EditFormat, DisplayFormat, std::move(OnDataAccess)) {
  _tcscpy(mValue, Default);
  SupportCombo = false;
}

const TCHAR* DataFieldString::SetAsString(const TCHAR* Value) {
  Set(Value);
  return mValue;
}

void DataFieldString::Set(const TCHAR* Value) {
  _tcscpy(mValue, Value);
}

const TCHAR* DataFieldString::GetAsString() {
  return mValue;
}

bool DataFieldString::CreateKeyboard() {
  dlgTextEntryShowModal(mValue, std::size(mValue));
  if (!GetDetachGUI()) {
    mOnDataAccess(this, daChange);
  }
  return true;
}
