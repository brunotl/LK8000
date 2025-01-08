/*
 * LK8000 Tactical Flight Computer -  WWW.LK8000.IT
 * Released under GNU/GPL License v.2 or later
 * See CREDITS.TXT file for authors and copyrights
 *
 * File:   DataFieldEnum.h
 */

#ifndef _FORM_DATAFIELDENUM_H_
#define _FORM_DATAFIELDENUM_H_

#include "DataField.h"
#include <vector>
#include <string>

class DataFieldEnum : public DataField {
 private:
  unsigned int mValue;

  struct DataFieldEnumEntry {
    unsigned int index;
    tstring mText;
    tstring mLabel;
  };

  std::vector<DataFieldEnumEntry> mEntries;

 public:
  DataFieldEnum(WndProperty& Owner, const char* EditFormat, const char* DisplayFormat, int Default,
                DataAccessCallback_t&& OnDataAccess);
  ~DataFieldEnum();

  void Clear() override;
  void Inc() override;
  void Dec() override;
  int CreateComboList() override;

  void addEnumTextNoLF(const TCHAR* Text) override;
  void addEnumText(const TCHAR* Text, const TCHAR* Label) override;
  void setEnumLabel(int idx, const TCHAR* Label) override;

  size_t getCount() const override {
    return mEntries.size();
  }

  void removeLastEnum() override;

  int Find(const TCHAR* Text) override;

  int GetAsInteger() override;
  const TCHAR* GetAsString() override;
  const TCHAR* GetAsDisplayString() override;
  bool GetAsBoolean() override;

  void Set(unsigned Value) override;

  void Set(int Value) override { 
    Set((unsigned)Value);
  }

  void Set(bool Value) override {
    Set(Value ? 1 : 0);
  }

  void Set(const TCHAR* Value) override {
    Set(Find(Value));
  }

  int SetAsInteger(int Value) override;
  void Sort(int startindex = 0) override;
};

#endif  // _FORM_DATAFIELDENUM_H_
