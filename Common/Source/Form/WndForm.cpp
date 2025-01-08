/*
   LK8000 Tactical Flight Computer -  WWW.LK8000.IT
   Released under GNU/GPL License v.2 or later
   See CREDITS.TXT file for authors and copyrights

   $Id: WndForm.cpp,v 1.1 2023/10/21 10:35:29 root Exp root $
*/

#include "WndForm.h"
#include "externs.h"
#include "Message.h"
#include "OS/RotateScreen.h"
#include "Event/Event.h"

WndForm::WndForm(const TCHAR* Name, const TCHAR* Caption, int X, int Y, int Width, int Height, bool Modal)
    : WindowControl(NULL, Name, X, Y, Width, Height, false), mModal(Modal) {
  mClientWindow = NULL;
  mOnTimerNotify = NULL;
  mOnKeyDownNotify = NULL;
  mOnKeyUpNotify = NULL;
  mOnUser = NULL;
  mColorTitle = RGB_MENUTITLEBG;
  mhTitleFont = GetFont();
  mhBrushTitle = LKBrush_Black;
  mClientRect.top = (mBorderKind & BORDERTOP) ? DLGSCALE(1) : 0;
  mClientRect.left = (mBorderKind & BORDERLEFT) ? DLGSCALE(1) : 0;
  mClientRect.bottom = Height - ((mBorderKind & BORDERBOTTOM) ? DLGSCALE(1) : 0);
  mClientRect.right = Width - ((mBorderKind & BORDERRIGHT) ? DLGSCALE(1) : 0);
  mTitleRect.top = 0;
  mTitleRect.left = 0;
  mTitleRect.bottom = 0;
  mTitleRect.right = Width;
  mClientWindow = new WindowControl(this, TEXT(""), mClientRect.top, mClientRect.left,
                                    mClientRect.right - mClientRect.left, mClientRect.bottom - mClientRect.top);
  mClientWindow->SetBackColor(RGB_WINBACKGROUND);
  mClientWindow->SetCanFocus(false);
  SetCaption(Caption);
  mModalResult = 0;
}

WndForm::~WndForm() {
  Destroy();
}

void WndForm::Destroy() {
  WindowControl::Destroy();  // delete all childs
}

void WndForm::AddClient(WindowControl* Client) {
  if (mClientWindow != NULL) {
    mClientWindow->AddClient(Client);  // add it to the clientarea window
  } else {
    WindowControl::AddClient(Client);
  }
}

FontReference WndForm::SetTitleFont(FontReference Value) {
  FontReference res = mhTitleFont;
  mhTitleFont = Value;
  return res;
}

int WndForm::ShowModal() {
  enterTime.Update();
  Message::ScopeBlockRender BlockRender;
  ScopeLockScreen LockScreen;
  SetVisible(true);
  SetToForeground();
  mModalResult = 0;
  Window* oldFocus = main_window->GetFocusedWindow();
  if (!oldFocus || !oldFocus->IsChild(this)) {
    FocusNext(NULL);
  }
  Redraw();
  LKASSERT(event_queue);
#if defined(ANDROID) || defined(USE_POLL_EVENT) || defined(ENABLE_SDL) || defined(NON_INTERACTIVE)
  EventLoop loop(*event_queue, *main_window);
#else
  EventLoop loop(*event_queue);
#endif
  Event event;
  while (mModalResult == 0 && loop.Get(event)) {
    if (mModal && !main_window->FilterEvent(event, this)) {
      continue;
    }
    if (event.IsKeyDown()) {
#ifndef WIN32
      Window* w = GetFocusedWindow();
      if (w == nullptr) {
        w = this;
      }
      if (w->OnKeyDown(event.GetKeyCode())) {
        continue;
      }
      if (OnKeyDownNotify(GetFocusedWindow(), event.GetKeyCode())) {
        continue;
      }
      if (event.GetKeyCode() == KEY_ESCAPE) {
        mModalResult = mrCancel;
        continue;
      }
#ifdef USE_LINUX_INPUT
      if (event.GetKeyCode() == KEY_POWER) {
        mModalResult = mrCancel;
        continue;
      }
#endif
#endif
    }
    loop.Dispatch(event);
  }
  if (oldFocus) {
    oldFocus->SetFocus();
  }
  main_window->UnGhost();
#ifdef USE_GDI
  MapWindow::RequestFastRefresh();
#endif
  return mModalResult;
}

void WndForm::Paint(LKSurface& Surface) {
  if (!IsVisible())
    return;
  const TCHAR* szCaption = GetWndText();
  size_t nChar = _tcslen(szCaption);
  if (nChar > 0) {
    Surface.SetTextColor(RGB_MENUTITLEFG);
    Surface.SetBkColor(mColorTitle);
    Surface.SetBackgroundTransparent();
    const auto oldFont = Surface.SelectObject(mhTitleFont);
    Surface.FillRect(&mTitleRect, mhBrushTitle);
    RECT rcLine = GetClientRect();
    rcLine.top = mTitleRect.bottom;
    rcLine.bottom = mClientRect.top;
    Surface.FillRect(&rcLine, LKBrush_Green);
    Surface.DrawText(mTitleRect.left + DLGSCALE(2), mTitleRect.top, szCaption);
    Surface.SelectObject(oldFont);
  }
  PaintBorder(Surface);
}

void WndForm::SetCaption(const TCHAR* Value) {
  const TCHAR* szCaption = GetWndText();
  bool bRedraw = false;
  if (Value == NULL) {
    if (szCaption[0] != _T('\0')) {
      SetWndText(_T(""));
      bRedraw = true;
    }
  } else if (_tcscmp(szCaption, Value) != 0) {
    SetWndText(Value);
    bRedraw = true;
  }
  RECT rcClient = mClientRect;
  size_t nChar = Value ? _tcslen(Value) : 0;
  if (nChar > 0) {
    SIZE tsize = {0, 0};
    LKWindowSurface Surface(*this);
    const auto oldFont = Surface.SelectObject(mhTitleFont);
    Surface.GetTextSize(Value, &tsize);
    Surface.SelectObject(oldFont);
    mTitleRect.bottom = mTitleRect.top + tsize.cy;
    rcClient.top = mTitleRect.bottom + DLGSCALE(1);
  } else {
    mTitleRect.bottom = 0;
    rcClient.top = (mBorderKind & BORDERTOP) ? DLGSCALE(1) : 0;
  }
  if (!EqualRect(&mClientRect, &rcClient)) {
    mClientRect = rcClient;
    if (mClientWindow) {
      mClientWindow->FastMove(mClientRect);
      mClientWindow->SetTopWnd();
    }
    bRedraw = true;
  }
  if (bRedraw) {
    Redraw();
  }
}

int WndForm::SetBorderKind(int Value) {
  RECT rcClient = GetClientRect();
  if (mTitleRect.bottom > 0) {
    rcClient.top = mTitleRect.bottom + DLGSCALE(1);
  } else {
    rcClient.top += (Value & BORDERTOP) ? DLGSCALE(1) : 0;
  }
  rcClient.left += (Value & BORDERLEFT) ? DLGSCALE(1) : 0;
  rcClient.bottom -= (Value & BORDERBOTTOM) ? DLGSCALE(1) : 0;
  rcClient.right -= (Value & BORDERRIGHT) ? DLGSCALE(1) : 0;
  if (!EqualRect(&mClientRect, &rcClient)) {
    mClientRect = rcClient;
    if (mClientWindow) {
      mClientWindow->FastMove(mClientRect);
      mClientWindow->SetTopWnd();
    }
  }
  return WindowControl::SetBorderKind(Value);
}

LKColor WndForm::SetForeColor(const LKColor& Value) {
  if (mClientWindow)
    mClientWindow->SetForeColor(Value);
  return WindowControl::SetForeColor(Value);
}

LKColor WndForm::SetBackColor(const LKColor& Value) {
  if (mClientWindow)
    mClientWindow->SetBackColor(Value);
  return WindowControl::SetBackColor(Value);
}

void WndForm::SetFont(FontReference Value) {
  if (mClientWindow) {
    mClientWindow->SetFont(Value);
  }
  WindowControl::SetFont(Value);
}

void WndForm::Show() {
  main_window->UnGhost();
  WindowControl::Show();
  SetToForeground();
}

bool WndForm::OnKeyDownNotify(Window* pWnd, unsigned KeyCode) {
  if (mOnKeyDownNotify && (mOnKeyDownNotify)(this, KeyCode)) {
    return true;
  }
  if (ActiveControl) {
    WindowControl* pCtrl = ActiveControl->GetParent();
    if (pCtrl) {
      switch (KeyCode) {
        case KEY_UP:
          pCtrl->FocusPrev(ActiveControl);
          return true;
        case KEY_DOWN:
          pCtrl->FocusNext(ActiveControl);
          return true;
      }
    }
  }
  if (KeyCode == KEY_ESCAPE) {
    mModalResult = mrCancel;
    return true;
  }
  return false;
}
