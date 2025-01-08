/*
 * LK8000 Tactical Flight Computer -  WWW.LK8000.IT
 * Released under GNU/GPL License v.2 or later
 * See CREDITS.TXT file for authors and copyrights
 *
 * File:   DataFieldEnum.cpp
 */

#include "DataFieldEnum.h"
#include <algorithm>
#include <cassert>

DataFieldEnum::DataFieldEnum(WndProperty& Owner, const char* EditFormat, const char* DisplayFormat, int Default,
                             DataAccessCallback_t&& OnDataAccess)
    : DataField(Owner, EditFormat, DisplayFormat, std::move(OnDataAccess)), mValue(Default) {

  if (Default >= 0) {
    mValue = Default;
  } else {
    mValue = 0;
  }
  mOnDataAccess(this, daGet);
}

DataFieldEnum::~DataFieldEnum() {
  Clear();
}

void DataFieldEnum::Clear() {
  mEntries.clear();
  mValue = 0;
}

void DataFieldEnum::Inc() {
  if (mValue < mEntries.size() - 1) {
    mValue++;
    if (!GetDetachGUI()) {
      mOnDataAccess(this, daChange);
    }
  }
}

void DataFieldEnum::Dec() {
  if (mValue > 0) {
    mValue--;
    if (!GetDetachGUI()) {
      mOnDataAccess(this, daChange);
    }
  }
}

int DataFieldEnum::CreateComboList() {
  for (size_t i = 0; i < mEntries.size(); i++) {
    mComboList.push_back(i, mEntries[i].mText.c_str(), mEntries[i].mLabel.c_str());
  }
  mComboList.PropertyDataFieldIndexSaved = (mValue < mEntries.size()) ? mValue : 0;
  return mComboList.size();
}

void DataFieldEnum::addEnumTextNoLF(const TCHAR* Text) {
  tstring szTmp(Text);
  std::replace(szTmp.begin(), szTmp.end(), _T('\n'), _T(' '));
  mEntries.push_back({static_cast<unsigned>(mEntries.size()), std::move(szTmp), _T("")});
}

void DataFieldEnum::addEnumText(const TCHAR* Text, const TCHAR* Label) {
  mEntries.push_back({static_cast<unsigned>(mEntries.size()), Text, Label ? Label : _T("")});
}

void DataFieldEnum::setEnumLabel(int idx, const TCHAR* Label) {
  mEntries[idx].mLabel = Label ? Label : _T("");
}

int DataFieldEnum::Find(const TCHAR* Text) {
  auto it = std::find_if(mEntries.begin(), mEntries.end(),
                        [&](const DataFieldEnumEntry& item) {
                          return item.mText == Text;
                        });
  return (it != mEntries.end()) ? it->index : -1;
}

void DataFieldEnum::removeLastEnum() {
  mEntries.pop_back();
}

int DataFieldEnum::GetAsInteger() {
  return (mValue < mEntries.size()) ? mEntries[mValue].index : 0;
}

const TCHAR* DataFieldEnum::GetAsString() {
  return (mValue < mEntries.size()) ? mEntries[mValue].mText.c_str() : NULL;
}

const TCHAR* DataFieldEnum::GetAsDisplayString() {
  if (mValue < mEntries.size()) {
    const DataFieldEnumEntry& entry = mEntries[mValue];
    return entry.mLabel.empty() ? entry.mText.c_str() : entry.mLabel.c_str();
  }
  return NULL;
}

bool DataFieldEnum::GetAsBoolean() {
  LKASSERT(mEntries.size() == 2);
  return (mValue < mEntries.size()) ? mEntries[mValue].index : false;
}

void DataFieldEnum::Set(unsigned Value) {
  if (Value >= mEntries.size()) {
    Value = 0;
  }
  for (unsigned int i = 0; i < mEntries.size(); i++) {
    if (mEntries[i].index == Value) {
      if (mValue != Value) {
        mValue = Value;
        if (!GetDetachGUI()) {
          mOnDataAccess(this, daChange);
        }
      }
      return;
    }
  }
  mValue = 0;  // fallback
}

int DataFieldEnum::SetAsInteger(int Value) {
  Set(Value);
  return mEntries[mValue].index;
}

void DataFieldEnum::Sort(int startindex) {
  std::sort(std::next(mEntries.begin(), startindex), mEntries.end(),
            [](const DataFieldEnumEntry& a, const DataFieldEnumEntry& b) {
              return a.mText < b.mText;
            });
}
