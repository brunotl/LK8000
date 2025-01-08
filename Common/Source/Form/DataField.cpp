/*
 * LK8000 Tactical Flight Computer -  WWW.LK8000.IT
 * Released under GNU/GPL License v.2 or later
 * See CREDITS.TXT file for authors and copyrights
 *
 * File:   DataField.cpp
 */

#include "externs.h"
#include "utils/array_adaptor.h"
#include "LocalPath.h"
#include <algorithm>
#include <cmath>
#include <cassert>
#include "utils/stringext.h"
#include "Dialogs.h"
#include "Util/Clamp.hpp"
#include "DataField.h"
#include "WindowControls.h"
#include "ComboList.h"


static
void __Dummy(DataField *Sender, DataField::DataAccessKind_t Mode) { }

// DataField class implementation
DataField::DataField(WndProperty& Owner, const char* EditFormat, const char* DisplayFormat,
                     DataAccessCallback_t&& OnDataAccess)
    : mOwnerProperty(Owner), mOnDataAccess(std::forward<DataAccessCallback_t>(OnDataAccess))
{
  from_utf8(EditFormat, mEditFormat);
  from_utf8(DisplayFormat, mDisplayFormat);
  from_utf8("", mUnits); // blank units

  if (!mOnDataAccess) {
    mOnDataAccess = __Dummy;
  }
}

void DataField::addEnumList(std::initializer_list<const TCHAR*>&& list) {
  for (auto& item : list) {
    addEnumText(item);
  }
}

void DataField::Clear() {
  // need to implement in derived class ...
  LKASSERT(false);
}

void DataField::Special() {
  if (!GetDetachGUI()) {
    mOnDataAccess(this, daSpecial);
  }
}

void DataField::Inc() {
  if (!GetDetachGUI()) {
    mOnDataAccess(this, daInc);
  }
}

void DataField::Dec() {
  if (!GetDetachGUI()) {
    mOnDataAccess(this, daDec);
  }
}

void DataField::GetData() {
  if (!GetDetachGUI()) {
    mOnDataAccess(this, daGet);
  }
}

void DataField::SetData() {
  if (!GetDetachGUI()) {
    mOnDataAccess(this, daPut);
  }
}

void DataField::SetDisplayFormat(const TCHAR* Value) {
  LKASSERT(_tcslen(Value) <= FORMATSIZE);
  _tcscpy(mDisplayFormat, Value);
}

void DataField::SetEditFormat(const TCHAR* Value) {
  LKASSERT(_tcslen(Value) <= FORMATSIZE);
  _tcscpy(mEditFormat, Value);
}

void DataField::CopyString(TCHAR* szbuffOut, bool bFormatted) {
  unsigned iLen = 0;
  const TCHAR* str = bFormatted ? GetAsDisplayString() : GetAsString();
  if (str) {  // null leaves iLen=0
    iLen = std::min<unsigned>(_tcslen(str), ComboList::ITEMMAX - 1);
    LK_tcsncpy(szbuffOut, str, iLen);
  }
  szbuffOut[iLen] = '\0';
}

int DataField::CreateComboListStepping() {  // for DataFieldInteger and DataFieldFloat
  // builds ItemList[] by calling CreateItem for each item in list
  // sets ComboPopupItemSavedIndex (global)
  // returns ItemCount
  constexpr double ComboListInitValue = -99999;
  constexpr double ComboFloatPrec = 0.0000001; // rounds float errors to this precision

  double fNext = ComboListInitValue;
  double fCurrent = ComboListInitValue;
  double fLast = ComboListInitValue;
  TCHAR sTemp[ComboList::ITEMMAX];

  mComboList.ItemIndex = -1;

  size_t iListCount = 0;
  int iSelectedIndex = -1;
  int iStepDirection = 1;  // for integer & float step may be negative

  SetDisableSpeedUp(true);
  SetDetachGUI(true);  // disable display of inc/dec/change values

  tstring SavedValueFormatted = GetAsDisplayString();
  tstring SavedValue = GetAsString();

  // get step direction for int & float so we can detect if we skipped the value while iterating later
  double fSavedValue = GetAsFloat();
  Inc();
  double fBeforeDec = GetAsFloat();
  Dec();
  double fAfterDec = GetAsFloat();

  if (fAfterDec < fBeforeDec) {
    iStepDirection = 1;
  }
  else {
    iStepDirection = -1;
  }

  // reset datafield to top of list (or for large floats, away from selected item so it will be in the middle)
  for (iListCount = 0; iListCount < ComboList::LISTMAX / 2; iListCount++) {
    // for floats, go half way down only 100222
    Dec();
    fNext = GetAsFloat();

    if (fNext == fCurrent)  // we're at start of the list
      break;
    if (fNext == fLast)  // don't repeat Yes/No/etc  (is this needed w/out Bool?)
      break;

    fLast = fCurrent;
    fCurrent = fNext;
  }  // loop

  fNext = ComboListInitValue;
  fCurrent = ComboListInitValue;
  fLast = ComboListInitValue;

  fCurrent = GetAsFloat();
  mComboList.clear();

  // if we stopped before hitting start of list create <<Less>> value at top of list
  if (iListCount == ComboList::LISTMAX / 2)  // 100222
  {                                         // this data index item is checked on close of dialog
    mComboList.push_back(ComboList::ReopenLESSDataIndex, _T("<<More Items>>"), _T("<<More Items>>"));
  }

  // now we're at the beginning of the list, so load forward until end
  for (iListCount = 0; iListCount < ComboList::LISTMAX - 3; iListCount++) {  
    // stop at LISTMAX-3 b/c it may make an additional item if it's "off step", and
    // potentially two more items for <<More>> and << Less>>

    // test if we've stepped over the selected value which was not a multiple of the "step"
    if (iSelectedIndex == -1) {
      // not found yet so check if we've stepped over it
      if ((iStepDirection * GetAsFloat()) > (fSavedValue + ComboFloatPrec * iStepDirection)) {
        // step was too large, we skipped the selected value, so add it now
        mComboList.push_back(ComboList::Null, SavedValue.c_str(), SavedValueFormatted.c_str());
        iSelectedIndex = mComboList.size() - 1;
      }

    }  // endif iSelectedIndex == -1

    if (iSelectedIndex == -1 && fabs(fCurrent - fSavedValue) < ComboFloatPrec) {  // selected item index
      iSelectedIndex = mComboList.size() - 1;
    }

    CopyString(sTemp, true);  // can't call GetAsString & GetAsStringFormatted together (same output buffer)
    mComboList.push_back(ComboList::Null, GetAsString(), sTemp);

    Inc();
    fNext = GetAsFloat();

    if (fNext == fCurrent) {  // we're at start of the list
      break;
    }

    if (fNext == fLast && mComboList.size() > 0) {  // we're at the end of the range
      break;
    }

    fLast = fCurrent;
    fCurrent = fNext;
  }

  // if we stopped before hitting end of list create <<More>> value at end of list
  if (iListCount == ComboList::LISTMAX - 3) {  // this data index item is checked on close of dialog
    mComboList.push_back(ComboList::ReopenMOREDataIndex, _T("<<More Items>>"), _T("<<More Items>>"));
  }

  SetDisableSpeedUp(false);
  SetDetachGUI(false);  // disable dispaly of inc/dec/change values

  if (iSelectedIndex >= 0) {
    SetAsFloat(fSavedValue);
  }
  mComboList.PropertyDataFieldIndexSaved = iSelectedIndex;

  return mComboList.size();
}
