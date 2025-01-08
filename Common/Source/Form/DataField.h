/*
 * LK8000 Tactical Flight Computer -  WWW.LK8000.IT
 * Released under GNU/GPL License v.2 or later
 * See CREDITS.TXT file for authors and copyrights
 *
 * File:   DataField.h
 */

#ifndef _FORM_DATAFIELD_H_
#define _FORM_DATAFIELD_H_

#include "Screen/LKWindowSurface.h"
#include "Screen/BrushReference.h"
#include "Screen/PenReference.h"
#include "Screen/FontReference.h"
#include "Window/WndCtrlBase.h"
#include "LKObjects.h"
#include <tchar.h>
#include <string.h>
#include <functional>
#include <list>
#include <vector>
#include "Time/PeriodClock.hpp"
#include "ComboList.h"

#define FORMATSIZE 32
#define UNITSIZE 10
#define OUTBUFFERSIZE 128

class WndProperty;

class DataField {
 public:
  enum DataAccessKind_t {
    daGet,
    daPut,
    daChange,
    daInc,
    daDec,
    daSpecial,
  };

  using DataAccessCallback_t = std::function<void(DataField*, DataAccessKind_t)>;

  DataField(WndProperty& Owner, const char* EditFormat, const char* DisplayFormat, DataAccessCallback_t&& OnDataAccess);
  virtual ~DataField() {}

  virtual void Clear();

  virtual void Special();
  virtual void Inc();
  virtual void Dec();

  virtual void GetData();
  virtual void SetData();

  virtual bool GetAsBoolean() {
    assert(false);
    return false;
  }
  virtual int GetAsInteger() {
    assert(false);
    return 0;
  }
  virtual double GetAsFloat() {
    assert(false);
    return 0;
  }
  virtual const TCHAR* GetAsString() {
    assert(false);
    return NULL;
  }
  virtual const TCHAR* GetAsDisplayString() { 
    return GetAsString();
  }

  virtual bool SetAsBoolean(bool Value) {
    assert(false);
    return false;
  }
  virtual int SetAsInteger(int Value) {
    assert(false);
    return 0;
  }
  virtual double SetAsFloat(double Value) {
    assert(false);
    return 0.0;
  }
  virtual const TCHAR* SetAsString(const TCHAR* Value) {
    assert(false);
    return NULL;
  }

  virtual void Set(bool Value) { assert(false); }
  virtual void Set(int Value) { assert(false); }
  virtual void Set(double Value) { assert(false); }
  virtual void Set(const TCHAR* Value) { assert(false); }
  virtual void Set(unsigned Value) { assert(false); }

  virtual int SetMin(int Value) {
    assert(false);
    return 0;
  }
  virtual double SetMin(double Value) {
    assert(false);
    return false;
  }
  virtual int SetStep(int Value) {
    assert(false);
    return 0;
  }
  virtual double SetStep(double Value) {
    assert(false);
    return false;
  }
  virtual int SetMax(int Value) {
    assert(false);
    return 0;
  }
  virtual double SetMax(double Value) {
    assert(false);
    return 0;
  }
  void SetUnits(const TCHAR* text) { _tcscpy(mUnits, text); }
  const TCHAR* GetUnits() const { return mUnits; }

  virtual void addEnumText(const TCHAR* Text, const TCHAR* Label = nullptr) { assert(false); }
  virtual void setEnumLabel(int idx, const TCHAR* Label) { assert(false); }
  virtual void addEnumTextNoLF(const TCHAR* Text) { assert(false); }
  virtual void Sort(int startindex = 0) { assert(false); }

  void addEnumList(std::initializer_list<const TCHAR*>&& list);

  virtual size_t getCount() const {
    assert(false);
    return 0;
  }
  virtual void removeLastEnum() { assert(false); }

  virtual int Find(const TCHAR* Text) {
    assert(false);
    return -1;
  }

  void Use() { mUsageCounter++; }

  int Unuse() {
    mUsageCounter--;
    return mUsageCounter;
  }

  void SetDisplayFormat(const TCHAR* Value);
  void SetEditFormat(const TCHAR* Value);
  void SetDisableSpeedUp(bool bDisable) { mDisableSpeedup = bDisable; }  // allows combolist to iterate all values
  bool GetDisableSpeedUp() { return mDisableSpeedup; }
  void SetDetachGUI(bool bDetachGUI) {
    mDetachGUI = bDetachGUI;
  }  // allows combolist to iterate all values w/out triggering external events
  bool GetDetachGUI() { return mDetachGUI; }
  virtual int CreateComboList() { return 0; }
  virtual bool CreateKeyboard() { return false; }

  ComboList* GetCombo() { return &mComboList; }

  virtual int SetFromCombo(int iDataFieldIndex, const TCHAR* sValue) {
    return SetAsInteger(iDataFieldIndex);
  }

  void CopyString(TCHAR* szStringOut, bool bFormatted);

  bool SupportCombo = false;  // all Types dataField support combolist except DataFieldString.

  WndProperty& GetOwner() const { return mOwnerProperty; }

 protected:
  TCHAR mEditFormat[FORMATSIZE + 1];
  TCHAR mDisplayFormat[FORMATSIZE + 1];
  TCHAR mUnits[UNITSIZE + 1];

  WndProperty& mOwnerProperty;
  DataAccessCallback_t mOnDataAccess;

  ComboList mComboList;
  int CreateComboListStepping();

 protected:
  bool DataFieldKeyUp = false;

 private:
  int mUsageCounter = 0;
  bool mDisableSpeedup = false;
  bool mDetachGUI = false;
};

#endif  // _FORM_DATAFIELD_H_
