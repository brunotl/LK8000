/*
   LK8000 Tactical Flight Computer -  WWW.LK8000.IT
   Released under GNU/GPL License v.2 or later
   See CREDITS.TXT file for authors and copyrights

   $Id: WndForm.h,v 1.1 2023/10/21 10:35:29 root Exp root $
*/

#ifndef _FORM_WNDFORM_H
#define _FORM_WNDFORM_H

#include "WindowControls.h"
#include "Time/PeriodClock.hpp"

#define mrOK             2
#define mrCancel         3

class WndForm : public WindowControl {
  using OnTimerNotify_t = std::function<bool(WndForm*)>;
  using OnKeyDownNotify_t = std::function<bool(WndForm*, unsigned)>;
  using OnKeyUpNotify_t = std::function<bool(WndForm*, unsigned)>;
  using OnUser_t = std::function<bool(WndForm*, unsigned)>;

 protected:
  int mModalResult;
  LKColor mColorTitle;
  BrushReference mhBrushTitle;
  FontReference mhTitleFont;
  WindowControl* mClientWindow;
  RECT mClientRect;
  RECT mTitleRect;
  bool mModal;
  OnTimerNotify_t mOnTimerNotify;
  OnKeyDownNotify_t mOnKeyDownNotify;
  OnKeyUpNotify_t mOnKeyUpNotify;
  OnUser_t mOnUser;

  void Paint(LKSurface& Surface) override;

 public:
  WndForm(const TCHAR* Name, const TCHAR* Caption, int X, int Y, int Width, int Height, bool Modal = true);
  ~WndForm();
  void Destroy() override;
  WindowControl* GetClientArea() override { return (mClientWindow ? mClientWindow : WindowControl::GetClientArea()); }
  WndForm* GetParentWndForm() override { return this; }
  void AddClient(WindowControl* Client) override;

  bool OnClose() override {
    mModalResult = mrCancel;
    WindowControl::OnClose();
    return true;
  }

  PeriodClock enterTime;
  int GetModalResult() { return mModalResult; }

  int SetModalResult(int Value) {
    mModalResult = Value;
    return Value;
  }

  FontReference SetTitleFont(FontReference Value);
  int ShowModal();
  void Show() override;
  void SetCaption(const TCHAR* Value) override;
  int SetBorderKind(int Value) override;
  LKColor SetForeColor(const LKColor& Value) override;
  LKColor SetBackColor(const LKColor& Value) override;
  void SetFont(FontReference Value) override;
  void SetKeyDownNotify(OnKeyDownNotify_t KeyDownNotify) { mOnKeyDownNotify = KeyDownNotify; }
  void SetKeyUpNotify(OnKeyUpNotify_t KeyUpNotify) { mOnKeyUpNotify = KeyUpNotify; }

  void SetTimerNotify(unsigned uTime, OnTimerNotify_t OnTimerNotify) {
    mOnTimerNotify = OnTimerNotify;
    if (OnTimerNotify && uTime > 0) {
      StartTimer(uTime);
    } else {
      StopTimer();
    }
  }

  void SetOnUser(OnUser_t OnUser) { mOnUser = OnUser; }
  void ReinitialiseLayout(const RECT& Rect) {}

 protected:
  bool OnKeyDownNotify(Window* pWnd, unsigned KeyCode);
  bool OnKeyUpNotify(Window* pWnd, unsigned KeyCode) { return (mOnKeyUpNotify && (mOnKeyUpNotify)(this, KeyCode)); }

  void OnTimer() override {
    if (mOnTimerNotify) {
      mOnTimerNotify(this);
    }
  }

  bool OnUser(unsigned id) override { return (mOnUser && mOnUser(this, id)); }

  void OnDestroy() override {
    mModalResult = mrCancel;
    WndCtrlBase::OnDestroy();
  }
};

#endif // _FORM_WNDFORM_H
