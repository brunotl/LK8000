/*
   LK8000 Tactical Flight Computer -  WWW.LK8000.IT
   Released under GNU/GPL License v.2 or later
   See CREDITS.TXT file for authors and copyrights

   $Id: WndFrame.h,v 1.1 2011/12/21 10:35:29 root Exp root $
*/
#ifndef _FORM_WNDFRAME_H
#define _FORM_WNDFRAME_H

#include "WindowControls.h"

class WndFrame : public WindowControl {
 public:
  WndFrame(WindowControl* Owner, const TCHAR* Name, int X, int Y, int Width, int Height)
      : WindowControl(Owner, Name, X, Y, Width, Height) {
    mLastDrawTextHeight = 0;
    mIsListItem = false;
    mLButtonDown = false;
    SetForeColor(GetParent()->GetForeColor());
    SetBackColor(GetParent()->GetBackColor());
    mCaptionStyle = DT_EXPANDTABS | DT_LEFT | DT_NOCLIP | DT_WORDBREAK;
  };

  UINT GetCaptionStyle() { return mCaptionStyle; };
  UINT SetCaptionStyle(UINT Value);

  int GetLastDrawTextHeight() { return mLastDrawTextHeight; };

  void SetIsListItem(bool Value) { mIsListItem = Value; };

  virtual bool OnLButtonDown(const POINT& Pos);

  virtual void Paint(LKSurface& Surface);

 protected:
  virtual bool OnKeyDown(unsigned KeyCode);

  bool mIsListItem;

  int mLastDrawTextHeight;
  UINT mCaptionStyle;
  bool mLButtonDown;
};

#endif // _FORM_WNDFRAME_H
