/*
   LK8000 Tactical Flight Computer -  WWW.LK8000.IT
   Released under GNU/GPL License v.2 or later
   See CREDITS.TXT file for authors and copyrights

   $Id: WindowControls.cpp,v 8.10 2010/12/13 01:17:08 root Exp root $
*/

#include "WindowControls.h"
#include "externs.h"

#ifndef USE_GDI
#include "Screen/SubCanvas.hpp"
#endif

//----------------------------------------------------------
// WindowControl Classes
//----------------------------------------------------------

WindowControl* ActiveControl = nullptr;
WindowControl* LastFocusControl = nullptr;

void InitWindowControlModule();

int WindowControl::InstCount = 0;

WindowControl::WindowControl(WindowControl* Owner, const TCHAR* Name, int X, int Y, int Width, int Height, bool Visible)
    : WndCtrlBase(Name) {
  mCanFocus = false;

  mReadOnly = false;

  mOwner = Owner ? Owner->GetClientArea() : NULL;
  // setup Master Window (the owner of all)
  mParentWndForm = NULL;
  if (Owner) {
    mParentWndForm = Owner->GetParentWndForm();
  }

  mDontPaintSelector = false;

  InitWindowControlModule();

  mColorBack = RGB_WINBACKGROUND;  // PETROL
  mColorFore = RGB_WINFOREGROUND;  // WHITE

  InstCount++;

  // if Owner is Not provided, use main_window
  ContainerWindow* WndOnwer =
      Owner ? static_cast<ContainerWindow*>(Owner->GetClientArea()) : static_cast<ContainerWindow*>(main_window.get());

  if (mOwner) {
    mOwner->CalcChildRect(X, Y, Width, Height);
  }
  LKASSERT(X + Width > 0);

  Create(WndOnwer, (RECT){X, Y, X + Width, Y + Height});
  SetTopWnd();
  SetFont(MapWindowFont);
  if (mOwner != NULL)
    mOwner->AddClient(this);

  SetBorderColor(RGB_BLACK);
  mBorderKind = 0;  // BORDERRIGHT | BORDERBOTTOM;

  if (Visible) {
    SetVisible(true);
  }
}

WindowControl::~WindowControl() {}

void WindowControl::Destroy() {
  for (WindowControl* pCtrl : mClients) {
    pCtrl->Destroy();
    delete pCtrl;
  }
  mClients.clear();

  if (LastFocusControl == this) {
    LastFocusControl = nullptr;
  }

  if (ActiveControl == this) {
    ActiveControl = nullptr;
  }

  WndCtrlBase::Destroy();

  InstCount--;
}

void WindowControl::OnSetFocus() {
  WndCtrlBase::OnSetFocus();
  Redraw();
  ActiveControl = this;
  LastFocusControl = this;
}

void WindowControl::OnKillFocus() {
  WndCtrlBase::OnKillFocus();
  Redraw();
  ActiveControl = nullptr;
}

bool WindowControl::OnPaint(LKSurface& Surface, const RECT& Rect) {
#ifndef USE_GDI
  Paint(Surface);
#else
  const int win_width = GetWidth();
  const int win_height = GetHeight();

  LKBitmapSurface MemSurface(Surface, win_width, win_height);

  HWND hChildWnd = NULL;
  if ((hChildWnd = GetWindow(_hWnd, GW_CHILD)) != NULL) {
    RECT Client_Rect;
    while ((hChildWnd = GetWindow(hChildWnd, GW_HWNDNEXT)) != NULL) {
      if (::IsWindowVisible(hChildWnd)) {
        ::GetWindowRect(hChildWnd, &Client_Rect);

        ::ScreenToClient(_hWnd, (LPPOINT)&Client_Rect.left);
        ::ScreenToClient(_hWnd, (LPPOINT)&Client_Rect.right);

        ::ExcludeClipRect(MemSurface, Client_Rect.left, Client_Rect.top, Client_Rect.right, Client_Rect.bottom);
        ::ExcludeClipRect(Surface, Client_Rect.left, Client_Rect.top, Client_Rect.right, Client_Rect.bottom);
      }
    }
  }
  Paint(MemSurface);

  Surface.Copy(0, 0, win_width, win_height, MemSurface, 0, 0);
#endif
  return true;
}

void WindowControl::SetTop(int Value) {
  if (GetTop() != Value) {
    Move(GetLeft(), Value);
  }
}

void WindowControl::SetLeft(int Value) {
  if (GetLeft() != Value) {
    Move(Value, GetTop());
  }
}

void WindowControl::SetHeight(unsigned int Value) {
  if (GetHeight() != Value) {
    Resize(GetWidth(), Value);
  }
}

void WindowControl::SetWidth(unsigned int Value) {
  if (GetWidth() != Value) {
    Resize(Value, GetHeight());
  }
}

WindowControl* WindowControl::GetCanFocus() {
  if (!IsVisible() || !IsEnabled()) {
    return NULL;
  }
  if (mCanFocus && !mReadOnly) {
    return (this);
  }

  for (WindowControl* w : mClients) {
    if (w && w->GetCanFocus()) {
      return (w);
    }
  }
  return NULL;
};

void WindowControl::CalcChildRect(int& x, int& y, int& cx, int& cy) const {
  // use negative value to space down or right items
  // -999 to stay on the same line or column
  // -998 to advance one line or column with spacing IBLSCALE 3
  // -997 to advance one line or column with spacing IBLSCALE 6
  // -991 to advance one or column line with spacing IBLSCALE 1
  // -992 to advance one or column line with spacing IBLSCALE 2
  // other -n   advance exactly ((height or width) * n).

  if (!mClients.empty()) {
    if (y < 0) {
      WindowControl* pPrev = mClients.back();
      switch (y) {
        case -999:  //@ 101008
          y = pPrev->GetTop();
          break;
        case -998:  //@ 101115
          y = (pPrev->GetBottom() + DLGSCALE(3));
          break;
        case -997:  //@ 101115
          y = (pPrev->GetBottom() + DLGSCALE(6));
          break;
        case -992:
          y = (pPrev->GetBottom() + DLGSCALE(2));
          break;
        case -991:
          y = (pPrev->GetBottom() + DLGSCALE(1));
          break;
        default:
          y = (pPrev->GetTop() - ((pPrev->GetHeight()) * y));
          break;
      }
    }
    if (x < 0) {
      WindowControl* pPrev = mClients.back();
      switch (x) {
        case -999:  //@ 101008
          x = pPrev->GetRight();
          break;
        case -998:  //@ 101115
          x = (pPrev->GetRight() + DLGSCALE(3));
          break;
        case -997:  //@ 101115
          x = (pPrev->GetRight() + DLGSCALE(6));
          break;
        case -992:
          x = (pPrev->GetRight() + DLGSCALE(2));
          break;
        case -991:
          x = (pPrev->GetRight() + DLGSCALE(1));
          break;
        default:
          x = (pPrev->GetRight() - ((pPrev->GetWidth()) * x));
          break;
      }
    }
  }

  if (y < 0)
    y = 0;
  if (x < 0)
    x = 0;

  // negative value for cx is right margin relative to parent;
  if (cx < 0) {
    cx = GetWidth() - x + cx;
  }
  // negative value for cy is bottom margin relative to parent;
  if (cy < 0) {
    cy = GetHeight() - y + cy;
  }
  LKASSERT(cx > 0);
  LKASSERT(cy > 0);
}

void WindowControl::AddClient(WindowControl* Client) {
  Client->SetFont(GetFont());
  mClients.push_back(Client);
}

WindowControl* WindowControl::FindChild(const TCHAR* Name) {
  if (Name) {
    if (_tcscmp(GetWndName(), Name) == 0) {
      return (this);
    }

    for (WindowControl* w : mClients) {
      WindowControl* res = w->FindChild(Name);
      if (res) {
        return res;
      }
    }
  }
  return NULL;
}

void WindowControl::ForEachChild(std::function<void(WindowControl*)> visitor) {
  for (WindowControl* w : mClients) {
    visitor(w);
    w->ForEachChild(visitor);
  }
}

void WindowControl::SetCaption(const TCHAR* Value) {
  const TCHAR* szCaption = GetWndText();
  if (Value == NULL) {
    if (szCaption[0] != _T('\0')) {
      SetWndText(_T(""));
      Redraw();
    }
  } else if (_tcscmp(szCaption, Value) != 0) {
    SetWndText(Value);
    Redraw();
  }
}

bool WindowControl::SetCanFocus(bool Value) {
  bool res = mCanFocus;
  mCanFocus = Value;
  return (res);
}

int WindowControl::GetBorderKind() {
  return (mBorderKind);
}

int WindowControl::SetBorderKind(int Value) {
  int res = mBorderKind;
  if (mBorderKind != Value) {
    mBorderKind = Value;
    Redraw();
  }
  return (res);
}

bool WindowControl::SetReadOnly(bool Value) {
  bool res = mReadOnly;
  if (mReadOnly != Value) {
    mReadOnly = Value;

    Redraw();
  }
  return (res);
}

LKColor WindowControl::SetForeColor(const LKColor& Value) {
  LKColor res = mColorFore;
  if (mColorFore != Value) {
    mColorFore = Value;
    if (IsVisible()) {
      Redraw();
    }
  }
  return res;
}

LKColor WindowControl::SetBackColor(const LKColor& Value) {
  LKColor res = mColorBack;
  if (mColorBack != Value) {
    mColorBack = Value;
    mBrushBk.Create(mColorBack);
    if (IsVisible()) {
      Redraw();
    }
  }
  return res;
}

void WindowControl::PaintBorder(LKSurface& Surface) {
  if (mBorderKind != 0) {
    const RECT rcClient = GetClientRect();

    if (mBorderKind & BORDERTOP) {
      const RECT rcLine = {rcClient.left, rcClient.top, rcClient.right, rcClient.top + DLGSCALE(1)};
      Surface.FillRect(&rcLine, mBrushBorder);
    }
    if (mBorderKind & BORDERRIGHT) {
      const RECT rcLine = {rcClient.right - DLGSCALE(1), rcClient.top, rcClient.right, rcClient.bottom};
      Surface.FillRect(&rcLine, mBrushBorder);
    }
    if (mBorderKind & BORDERBOTTOM) {
      const RECT rcLine = {rcClient.left, rcClient.bottom - DLGSCALE(1), rcClient.right, rcClient.bottom};
      Surface.FillRect(&rcLine, mBrushBorder);
    }
    if (mBorderKind & BORDERLEFT) {
      const RECT rcLine = {rcClient.left, rcClient.top, rcClient.left + DLGSCALE(1), rcClient.bottom};
      Surface.FillRect(&rcLine, mBrushBorder);
    }
  }
}

void WindowControl::PaintSelector(LKSurface& Surface) {
#ifdef USE_OLD_SELECTOR
#define SELECTORWIDTH DLGSCALE(4)
  if (!mDontPaintSelector && mCanFocus && HasFocus()) {
    const auto oldPen = Surface.SelectObject(LKPen_Petrol_C2);

    const int Width = GetWidth();
    const int Height = GetHeight();

    Surface.DrawLine(Width - SELECTORWIDTH - 1, 0, Width - 1, 0, Width - 1, SELECTORWIDTH + 1);

    Surface.DrawLine(Width - 1, Height - SELECTORWIDTH - 2, Width - 1, Height - 1, Width - SELECTORWIDTH - 1,
                     Height - 1);

    Surface.DrawLine(SELECTORWIDTH + 1, Height - 1, 0, Height - 1, 0, Height - SELECTORWIDTH - 2);

    Surface.DrawLine(0, SELECTORWIDTH + 1, 0, 0, SELECTORWIDTH + 1, 0);

    Surface.SelectObject(oldPen);
  }
#endif
}

void WindowControl::Paint(LKSurface& Surface) {
  if (!IsVisible())
    return;

  Surface.SetBackgroundTransparent();
  RECT rc = GetClientRect();
  rc.right += 2;
  rc.bottom += 2;

  Surface.FillRect(&rc, GetBackBrush());

  // JMW added highlighting, useful for lists
  if (!mDontPaintSelector && mCanFocus && HasFocus()) {
    rc.right -= 2;
    rc.bottom -= 2;
    Surface.FillRect(&rc, LKBrush_Higlighted);
  }
  PaintBorder(Surface);
#ifdef USE_OLD_SELECTOR
  PaintSelector(Surface);
#endif
}

WindowControl* WindowControl::FocusNext(WindowControl* Sender) {
  WindowControl* W = NULL;
  if (Sender != NULL) {
    auto It = std::find(mClients.begin(), mClients.end(), Sender);
    if (It != mClients.end()) {
      ++It;
    }
    for (; !W && It != mClients.end(); ++It) {
      W = (*It)->GetCanFocus();
    }
  } else if (!mClients.empty()) {
    W = mClients.front()->GetCanFocus();
  }

  if (W) {
    W->SetFocus();
    return W;
  }

  if (GetParent() != NULL) {
    return (GetParent()->FocusNext(this));
  }

  return NULL;
}

WindowControl* WindowControl::FocusPrev(WindowControl* Sender) {
  WindowControl* W = NULL;
  if (Sender != NULL) {
    auto It = std::find(mClients.rbegin(), mClients.rend(), Sender);
    if (It != mClients.rend()) {
      ++It;
    }
    for (; !W && It != mClients.rend(); ++It) {
      W = (*It)->GetCanFocus();
    }
  } else if (!mClients.empty()) {
    W = mClients.back()->GetCanFocus();
  }

  if (W) {
    W->SetFocus();
    return W;
  }

  if (GetParent() != NULL) {
    return (GetParent()->FocusPrev(this));
  }

  return NULL;
}

void InitWindowControlModule() {
  static bool InitDone = false;

  if (InitDone)
    return;

  ActiveControl = nullptr;

  InitDone = true;
}
