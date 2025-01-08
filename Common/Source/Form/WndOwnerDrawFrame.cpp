/*
   LK8000 Tactical Flight Computer -  WWW.LK8000.IT
   Released under GNU/GPL License v.2 or later
   See CREDITS.TXT file for authors and copyrights

   $Id: WndOwnerDrawFrame.cpp,v 8.10 2010/12/13 01:17:08 root Exp root $
*/

#include "WndOwnerDrawFrame.h"

void WndOwnerDrawFrame::Paint(LKSurface& Surface) {
  if (IsVisible()) {
    WndFrame::Paint(Surface);
    const auto oldFont = Surface.SelectObject(GetFont());
    if (mOnPaintCallback != NULL) {
      (mOnPaintCallback)(this, Surface);
    }
    Surface.SelectObject(oldFont);
  }
}