/*
 * LK8000 Tactical Flight Computer -  WWW.LK8000.IT
 * Released under GNU/GPL License v.2 or later
 * See CREDITS.TXT file for authors and copyrights
 *
 * File:   DataFieldString.h
 */

#ifndef _FORM_DATAFIELD_STRING_H_
#define _FORM_DATAFIELD_STRING_H_

#include "DataField.h"

class DataFieldString : public DataField {
 private:
  TCHAR mValue[64];

 public:
  DataFieldString(WndProperty& Owner, const char* EditFormat, const char* DisplayFormat,
                  DataAccessCallback_t&& OnDataAccess);

  const TCHAR* SetAsString(const TCHAR* Value) override;
  void Set(const TCHAR* Value) override;

  const TCHAR* GetAsString() override;

  bool CreateKeyboard() override;
};

#endif  // _FORM_DATAFIELD_STRING_H_
