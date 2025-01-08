/*
 * LK8000 Tactical Flight Computer -  WWW.LK8000.IT
 * Released under GNU/GPL License v.2 or later
 * See CREDITS.TXT file for authors and copyrights
 *
 * File:   DataFieldInteger.h
 */

#ifndef _FORM_DATAFIELDINTEGER_H_
#define _FORM_DATAFIELDINTEGER_H_

#include "DataField.h"

class DataFieldInteger : public DataField {
 private:
  int mMin;
  int mMax;
  int mValue = 0;
  int mStep;
  PeriodClock mTmLastStep;
  int mSpeedup;
  TCHAR mOutBuf[OUTBUFFERSIZE + 1];

 protected:
  int SpeedUp(bool keyup);

 public:
  DataFieldInteger(WndProperty& Owner, const char* EditFormat, const char* DisplayFormat, int Min, int Max,
                   int Step, DataAccessCallback_t&& OnDataAccess);

  void Inc() override;
  void Dec() override;
  int CreateComboList() override;
  bool CreateKeyboard() override;

  bool GetAsBoolean() override;
  int GetAsInteger() override;
  double GetAsFloat() override;
  const TCHAR* GetAsString() override;
  const TCHAR* GetAsDisplayString() override;

  void Set(int Value) override;
  int SetMin(int Value) override {
    mMin = Value;
    return mMin;
  }
  int SetMax(int Value) override {
    mMax = Value;
    return mMax;
  }

  bool SetAsBoolean(bool Value) override;
  int SetAsInteger(int Value) override;
  double SetAsFloat(double Value) override;
  const TCHAR* SetAsString(const TCHAR* Value) override;
};

#endif  // _FORM_DATAFIELDINTEGER_H_
