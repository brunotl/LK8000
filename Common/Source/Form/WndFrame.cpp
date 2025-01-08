/*
   LK8000 Tactical Flight Computer -  WWW.LK8000.IT
   Released under GNU/GPL License v.2 or later
   See CREDITS.TXT file for authors and copyrights

   $Id: WndFrame.cpp,v 1.1 2023/10/21 10:35:29 root Exp root $
*/

#include "WndFrame.h"
#include "WndListFrame.h"
#include "Screen/LKBitmapSurface.h"

bool WndFrame::OnLButtonDown(const POINT& Pos) {
  mLButtonDown = true;
  if (!HasFocus()) {
    SetFocus();
    Redraw();
  }
  return false;
}

bool WndFrame::OnKeyDown(unsigned KeyCode) {
  if (mIsListItem && GetParent() != NULL) {
    return (((WndListFrame*)GetParent())->OnItemKeyDown(this, KeyCode));
  }
  return false;
}

void WndFrame::Paint(LKSurface& Surface) {
  if (!IsVisible())
    return;

  if (mIsListItem && GetParent() != NULL) {
    ((WndListFrame*)GetParent())->PrepareItemDraw();
  }

  WindowControl::Paint(Surface);

  const TCHAR* szCaption = GetWndText();
  const size_t nSize = _tcslen(szCaption);
  if (nSize > 0) {
    Surface.SetTextColor(GetForeColor());
    Surface.SetBkColor(GetBackColor());
    Surface.SetBackgroundTransparent();

    const auto oldFont = Surface.SelectObject(GetFont());

    RECT rc = GetClientRect();
    InflateRect(&rc, -2, -2);  // todo border width

    Surface.DrawText(szCaption, &rc, mCaptionStyle);

    mLastDrawTextHeight = rc.bottom - rc.top;

    Surface.SelectObject(oldFont);
  }
}

UINT WndFrame::SetCaptionStyle(UINT Value) {
  UINT res = mCaptionStyle;
  if (res != Value) {
    mCaptionStyle = Value;
    Redraw();
  }
  return (res);
}
