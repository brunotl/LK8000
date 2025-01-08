/*
   LK8000 Tactical Flight Computer -  WWW.LK8000.IT
   Released under GNU/GPL License v.2 or later
   See CREDITS.TXT file for authors and copyrights

   $Id: WndProperty.h,v 1.1 2011/12/21 10:35:29 root Exp root $
*/

#ifndef _FORM_WNDPROPERTY_H
#define _FORM_WNDPROPERTY_H

#include "WindowControls.h"
#include "LKObjects.h"
#include <functional>
#include <memory>

class DataField;
using DataFieldPtr = std::shared_ptr<DataField>;

#define STRINGVALUESIZE 128

class WndProperty : public WindowControl {
 public:
  using OnHelpCallback_t = std::function<void(WndProperty*)>;

 private:
  RECT mEditRect;

  FontReference mhValueFont;
  int mBitmapSize;
  int mCaptionWidth;
  RECT mHitRectUp;
  RECT mHitRectDown;
  bool mDownDown;
  bool mUpDown;
  bool mUseKeyboard;
  bool mMultiLine;

  void Paint(LKSurface& Surface) override;

  int CallSpecial();
  int IncValue();
  int DecValue();

  DataFieldPtr mDataField;
  tstring mValue;

  void UpdateButtonData();
  bool mDialogStyle;

  OnHelpCallback_t mOnHelpCallback;
  TCHAR* mHelpText = nullptr;

  void SetDataField(DataFieldPtr Value);

 public:
  WndProperty(WindowControl* Parent, TCHAR* Name, TCHAR* Caption, int X, int Y, int Width, int Height, int CaptionWidth,
              int MultiLine, OnHelpCallback_t&& Function);
  ~WndProperty();

  void Destroy() override;

  void SetHelpText(const TCHAR* Value);
  bool HasHelpText() const {
    return (mHelpText || mOnHelpCallback);
  }

  int OnHelp();

  bool SetReadOnly(bool Value) override;
  bool SetUseKeyboard(bool Value);

  void RefreshDisplay();

  void SetFont(FontReference Value) override;

  bool OnKeyDown(unsigned KeyCode) override;
  bool OnKeyUp(unsigned KeyCode) override;
  bool OnLButtonDown(const POINT& Pos) override;
  bool OnLButtonUp(const POINT& Pos) override;
  bool OnLButtonDblClick(const POINT& Pos) override;

  DataFieldPtr GetDataField() {
    return mDataField;
  }

  void SetText(const TCHAR* Value);
  int SetButtonSize(int Value);

  template<typename DataFieldT, typename ...Args>
  void CreateDataField(Args&&... args) {
    SetDataField(std::make_shared<DataFieldT>(*this, std::forward<Args>(args)...));
  }

};

int dlgComboPicker(WndProperty* pWnd);

#endif  // _FORM_WNDPROPERTY_H
