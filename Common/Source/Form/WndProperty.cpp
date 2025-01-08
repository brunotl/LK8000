/*
   LK8000 Tactical Flight Computer -  WWW.LK8000.IT
   Released under GNU/GPL License v.2 or later
   See CREDITS.TXT file for authors and copyrights

   $Id: WndProperty.cpp,v 8.10 2010/12/13 01:17:08 root Exp root $
*/

#include "WndProperty.h"
#include "ScreenGeometry.h"
#include "DataField.h"
#include "Dialogs.h"
#include "Event/Key.h"
#include "Bitmaps.h"
#include "RGB.h"

#define DEFAULTBORDERPENWIDTH DLGSCALE(1)

// returns true if it is a long press,
// otherwise returns false
static bool KeyTimer(bool isdown, unsigned thekey) {
  static PeriodClock fpsTimeDown;
  static unsigned savedKey = 0;

  if ((thekey == savedKey) && fpsTimeDown.CheckUpdate(2000)) {
    savedKey = 0;
    return true;
  }

  if (!isdown) {
    // key is released
  } else {
    // key is lowered
    if (thekey != savedKey) {
      fpsTimeDown.Update();
      savedKey = thekey;
    }
  }
  return false;
}

WndProperty::WndProperty(WindowControl* Parent, TCHAR* Name, TCHAR* Caption, int X, int Y, int Width, int Height,
                         int CaptionWidth, int MultiLine, OnHelpCallback_t&& Function)
    : WindowControl(Parent, Name, X, Y, Width, Height), mOnHelpCallback(std::move(Function)) {
  if (Caption) {
    SetCaption(Caption);
  }

  mDataField = nullptr;
  mDialogStyle = false;  // this is set by ::SetDataField()

  mUseKeyboard = false;
  mMultiLine = MultiLine;

  mhValueFont = GetFont();
  mCaptionWidth = CaptionWidth;

  mBitmapSize = DLGSCALE(32) / 2;
  if (mDialogStyle)
    mBitmapSize = 0;

  UpdateButtonData();

  mCanFocus = true;

  SetForeColor(GetParent()->GetForeColor());
  SetBackColor(GetParent()->GetBackColor());

  mDownDown = false;
  mUpDown = false;
}

WndProperty::~WndProperty() {
  if (mHelpText) {
    free(mHelpText);
    mHelpText = nullptr;
  }
}

void WndProperty::Destroy() {
  if (mDataField) {
    if (!mDataField->Unuse()) {
      delete mDataField;
      mDataField = nullptr;
    } else {
      // ASSERT(0);
    }
  }
  WindowControl::Destroy();
}

void WndProperty::SetHelpText(const TCHAR* Value) {
  if (mHelpText) {
    free(mHelpText);
    mHelpText = nullptr;
  }
  if (Value && Value[0]) {
    mHelpText = _tcsdup(Value);
  }
}

int WndProperty::OnHelp() {
  if (mHelpText) {
    dlgHelpShowModal(GetWndText(), mHelpText);
    return 1;
  }
  if (mOnHelpCallback) {
    (mOnHelpCallback)(this);
    return 1;
  }
  return 0;
}

void WndProperty::SetText(const TCHAR* Value) {
  if (!Value && mValue != _T("")) {
    mValue.clear();
    Redraw();
  } else if (Value && mValue != Value) {
    mValue = Value;
    Redraw();
  }
}

void WndProperty::SetFont(FontReference Value) {
  WindowControl::SetFont(Value);
  mhValueFont = Value;
}

void WndProperty::UpdateButtonData() {
  if (mCaptionWidth != 0) {
    mEditRect.left = mCaptionWidth;
    mEditRect.top = (DEFAULTBORDERPENWIDTH + 1);

    mEditRect.right = mEditRect.left + GetWidth() - mCaptionWidth - (DEFAULTBORDERPENWIDTH + 1) - mBitmapSize;
    mEditRect.bottom = mEditRect.top + GetHeight() - 2 * (DEFAULTBORDERPENWIDTH + 1);
  } else {
    mEditRect.left = mBitmapSize + (DEFAULTBORDERPENWIDTH + 2);
    mEditRect.top = (GetHeight() / 2) - 2 * (DEFAULTBORDERPENWIDTH + 1);
    mEditRect.right = mEditRect.left + GetWidth() - 2 * ((DEFAULTBORDERPENWIDTH + 1) + mBitmapSize);
    mEditRect.bottom = mEditRect.top + (GetHeight() / 2);
  }

  mHitRectDown.left = mEditRect.left - mBitmapSize;
  mHitRectDown.top = mEditRect.top + (mEditRect.top) / 2 - (mBitmapSize / 2);
  mHitRectDown.right = mHitRectDown.left + mBitmapSize;
  mHitRectDown.bottom = mHitRectDown.top + mBitmapSize;

  mHitRectUp.left = GetWidth() - (mBitmapSize + 2);
  mHitRectUp.top = mHitRectDown.top;
  mHitRectUp.right = mHitRectUp.left + mBitmapSize;
  mHitRectUp.bottom = mHitRectUp.top + mBitmapSize;
}

bool WndProperty::SetUseKeyboard(bool Value) {
  return mUseKeyboard = Value;
}

int WndProperty::SetButtonSize(int Value) {
  int res = mBitmapSize;

  if (mBitmapSize != Value) {
    mBitmapSize = Value;
    UpdateButtonData();

    if (IsVisible()) {
      Redraw();
    }
  }
  return res;
};

bool WndProperty::SetReadOnly(bool Value) {
  bool Res = WindowControl::SetReadOnly(Value);
  if (Value) {
    SetButtonSize(0);
    SetCanFocus(true);
  } else if (mDataField && !mDialogStyle) {
    SetButtonSize(DLGSCALE(32) / 2);
  }
  if (Res != Value) {
    Redraw();
  }
  return Res;
}

bool WndProperty::OnKeyDown(unsigned KeyCode) {
  switch (KeyCode) {
    case KEY_RIGHT:
      IncValue();
      return true;
    case KEY_LEFT:
      DecValue();
      return true;
    case KEY_RETURN:
      if (this->mDialogStyle) {
        if (OnLButtonDown((POINT){0, 0})) {
          return true;
        }
      } else {
        if (KeyTimer(true, KeyCode) && OnHelp()) {
          return true;
        }
      }
    default:
      break;
  }
  return false;
}

bool WndProperty::OnKeyUp(unsigned KeyCode) {
  if (KeyTimer(false, KeyCode & 0xffff)) {
    // activate tool tips if hit return for long time
    if ((KeyCode & 0xffff) == KEY_RETURN) {
      if (OnHelp())
        return true;
    }
  } else if ((KeyCode & 0xffff) == KEY_RETURN) {
    if (CallSpecial()) {
      return true;
    }
  }
  return false;
}

extern BOOL dlgKeyboard(WndProperty* theProperty);

bool WndProperty::OnLButtonDown(const POINT& Pos) {
  if (mDialogStyle) {
    if (!GetReadOnly())  // when they click on the label
    {
      if (!mUseKeyboard || !dlgKeyboard(this)) {
        dlgComboPicker(this);
      }
    } else {
      OnHelp();  // this would display xml file help on a read-only wndproperty if it exists
    }
  } else {
    if (!HasFocus()) {
      SetFocus();
    } else {
      mDownDown = (PtInRect(&mHitRectDown, Pos) != 0);

      if (mDownDown) {
        DecValue();
        Redraw(mHitRectDown);
      }

      mUpDown = (PtInRect(&mHitRectUp, Pos) != 0);

      if (mUpDown) {
        IncValue();
        Redraw(mHitRectUp);
      }
    }
  }
  return true;
}

bool WndProperty::OnLButtonDblClick(const POINT& Pos) {
  return OnLButtonDown(Pos);
}

bool WndProperty::OnLButtonUp(const POINT& Pos) {
  if (!mDialogStyle) {
    if (mDownDown) {
      mDownDown = false;
      Redraw(mHitRectDown);
    }
    if (mUpDown) {
      mUpDown = false;
      Redraw(mHitRectUp);
    }
  }
  return true;
}

int WndProperty::CallSpecial() {
  if (mDataField != NULL) {
    mDataField->Special();
    SetText(mDataField->GetAsString());
  }
  return 0;
}

int WndProperty::IncValue() {
  if (mDataField != NULL) {
    mDataField->Inc();
    SetText(mDataField->GetAsString());
  }
  return 0;
}

int WndProperty::DecValue() {
  if (mDataField != NULL) {
    mDataField->Dec();
    SetText(mDataField->GetAsString());
  }
  return 0;
}

void WndProperty::Paint(LKSurface& Surface) {
  //  RECT r;
  SIZE tsize;
  POINT org;

  if (!IsVisible()) {
    return;
  }

  PixelRect rc(GetClientRect());
  rc.right += 2;
  rc.bottom += 2;

  Surface.FillRect(&rc, GetBackBrush());

  if (HasFocus() && mCanFocus) {
    auto oldBrush = Surface.SelectObject(LK_HOLLOW_BRUSH);
    auto oldPen = Surface.SelectObject(LKPen_Higlighted);
    Surface.Rectangle(rc.left, rc.top, rc.right - 3, rc.bottom - 3);
    Surface.SelectObject(oldPen);
    Surface.SelectObject(oldBrush);
  }

  Surface.SetTextColor(GetForeColor());
  Surface.SetBkColor(GetBackColor());
  Surface.SetBackgroundTransparent();
  const auto oldFont = Surface.SelectObject(GetFont());

  const TCHAR* szCaption = GetWndText();
  const size_t nSize = _tcslen(szCaption);
  Surface.GetTextSize(szCaption, &tsize);
  if (nSize == 0) {
    tsize.cy = 0;  //@ 101115 BUGFIX
  }

  if (mCaptionWidth == 0) {
    org.x = mEditRect.left;
    org.y = mEditRect.top - tsize.cy;
  } else {
    org.x = mCaptionWidth - mBitmapSize - (tsize.cx + DLGSCALE(2));
    org.y = (GetHeight() - tsize.cy) / 2;
  }

  if (org.x < 1) {
    org.x = 1;
  }

  Surface.DrawText(org.x, org.y, szCaption);

  // these are button left and right icons for waypoint select, for example
  if (mDialogStyle)  // can't but dlgComboPicker here b/c it calls paint when combopicker closes too
  {                  // so it calls dlgCombopicker on the click/focus handlers for the wndproperty & label
                     // opening a window, each subwindow goes here once
  } else if (HasFocus() && !GetReadOnly()) {
    hBmpLeft32.Draw(Surface, mHitRectDown.left, mHitRectDown.top, mBitmapSize, mBitmapSize);
    hBmpRight32.Draw(Surface, mHitRectUp.left, mHitRectUp.top, mBitmapSize, mBitmapSize);
  }

  if ((mEditRect.right - mEditRect.left) > mBitmapSize) {
    auto oldBrush = Surface.SelectObject(GetReadOnly() ? LKBrush_LightGrey : LKBrush_White);
    auto oldPen = Surface.SelectObject(LK_BLACK_PEN);
    // Draw Text Bakground & Border
    Surface.Rectangle(mEditRect.left, mEditRect.top, mEditRect.right, mEditRect.bottom);
    // Draw Text Value

    RECT rcText = mEditRect;
    InflateRect(&rcText, -DLGSCALE(3), -1);

    Surface.SelectObject(mhValueFont);
    Surface.SetTextColor(RGB_BLACK);

    Surface.DrawText(mValue.c_str(), &rcText, DT_EXPANDTABS | (mMultiLine ? DT_WORDBREAK : DT_SINGLELINE | DT_VCENTER));

    Surface.SelectObject(oldPen);
    Surface.SelectObject(oldBrush);
  }
  Surface.SelectObject(oldFont);
}

void WndProperty::RefreshDisplay() {
  if (!mDataField) {
    return;
  }
  if (HasFocus()) {
    SetText(mDataField->GetAsString());
  } else {
    SetText(mDataField->GetAsDisplayString());
  }
  Redraw();
}

DataField* WndProperty::SetDataField(DataField* Value) {
  DataField* res = mDataField;

  if (mDataField != Value) {
    if (mDataField && !mDataField->Unuse()) {
      delete (mDataField);
      res = NULL;
    }

    Value->Use();
    mDataField = Value;
    mDataField->GetData();
    mDialogStyle = mDataField->SupportCombo || mUseKeyboard;

    if (GetReadOnly() || mDialogStyle) {
      SetButtonSize(0);
      this->SetCanFocus(true);
    } else if (mDataField && !mDialogStyle) {
      SetButtonSize(DLGSCALE(32) / 2);
    }

    RefreshDisplay();
  }
  return res;
}
