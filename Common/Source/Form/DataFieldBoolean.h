/*
 * LK8000 Tactical Flight Computer -  WWW.LK8000.IT
 * Released under GNU/GPL License v.2 or later
 * See CREDITS.TXT file for authors and copyrights
 *
 * File:   DataFieldBoolean.h
 */

#ifndef _FORM_DATAFIELD_BOOLEAN_H_
#define _FORM_DATAFIELD_BOOLEAN_H_

#include "DataField.h"

class DataFieldBoolean : public DataField {
 private:
  bool mValue = false;
  TCHAR mTextTrue[FORMATSIZE + 1];
  TCHAR mTextFalse[FORMATSIZE + 1];

 public:
  DataFieldBoolean(WndProperty& Owner, const char* EditFormat, const char* DisplayFormat,
                   DataAccessCallback_t&& OnDataAccess);

  void Inc() override;
  void Dec() override;
  int CreateComboList() override;

  bool GetAsBoolean() override;
  int GetAsInteger() override;
  double GetAsFloat() override;
  const TCHAR* GetAsString() override;

  void Set(int Value) override;
  void Set(bool Value) override;

  bool SetAsBoolean(bool Value) override;
  int SetAsInteger(int Value) override;
  double SetAsFloat(double Value) override;
  const TCHAR* SetAsString(const TCHAR* Value) override;
};

#endif  // _FORM_DATAFIELD_BOOLEAN_H_
