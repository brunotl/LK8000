/*
 * LK8000 Tactical Flight Computer -  WWW.LK8000.IT
 * Released under GNU/GPL License v.2 or later
 * See CREDITS.TXT file for authors and copyrights
 *
 * File:   DataFieldFloat.h
 */

#ifndef _FORM_DATAFIELD_FLOAT_H_
#define _FORM_DATAFIELD_FLOAT_H_

#include "DataField.h"
#include "Time/PeriodClock.hpp"

class DataFieldFloat : public DataField {
 private:
  double mMin;
  double mMax;
  double mStep;
  PeriodClock mTmLastStep;
  int mFine;
  int mSpeedup = 0;

 protected:
  double mValue = 0.;
  TCHAR mOutBuf[OUTBUFFERSIZE + 1];

  double SpeedUp(bool keyup);

 public:
  DataFieldFloat(WndProperty& Owner, const char* EditFormat, const char* DisplayFormat, double Min, double Max,
                 double Step, int Fine, DataAccessCallback_t&& OnDataAccess);

  void Inc() override;
  void Dec() override;
  int CreateComboList() override;
  bool CreateKeyboard() override;
  int SetFromCombo(int iDataFieldIndex, const TCHAR* sValue) override;

  bool GetAsBoolean() override;
  int GetAsInteger() override;
  double GetAsFloat() override;
  const TCHAR* GetAsString() override;
  const TCHAR* GetAsDisplayString() override;

  void Set(int Value) override { Set((double)Value); }

  void Set(double Value) override;
  double SetMin(double Value) override;
  double SetMax(double Value) override;
  double SetStep(double Value) override;

  bool SetAsBoolean(bool Value) override;
  int SetAsInteger(int Value) override;
  double SetAsFloat(double Value) override;
  const TCHAR* SetAsString(const TCHAR* Value) override;
};

#endif  // _FORM_DATAFIELD_FLOAT_H_
