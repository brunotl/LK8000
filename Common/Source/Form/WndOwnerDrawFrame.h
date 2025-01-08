/*
   LK8000 Tactical Flight Computer -  WWW.LK8000.IT
   Released under GNU/GPL License v.2 or later
   See CREDITS.TXT file for authors and copyrights

   $Id: WndOwnerDrawFrame.h,v 1.1 2011/12/21 10:35:29 root Exp root $
*/

#ifndef _FORM_WNDOWNERDRAWFRAME_H
#define _FORM_WNDOWNERDRAWFRAME_H

#include "WndFrame.h"

class WndOwnerDrawFrame : public WndFrame {
 public:
  using OnPaintCallback_t = std::function<void(WndOwnerDrawFrame*, LKSurface&)>;

  WndOwnerDrawFrame(WindowControl* Owner, TCHAR* Name, int X, int Y, int Width, int Height,
                    OnPaintCallback_t&& OnPaintCallback)
      : WndFrame(Owner, Name, X, Y, Width, Height), mOnPaintCallback(std::move(OnPaintCallback)) {
    SetForeColor(GetParent()->GetForeColor());
    SetBackColor(GetParent()->GetBackColor());
  };

  void SetOnPaintNotify(OnPaintCallback_t&& OnPaintCallback) { mOnPaintCallback = std::move(OnPaintCallback); }

 protected:
  OnPaintCallback_t mOnPaintCallback;
  virtual void Paint(LKSurface& Surface);
};

#endif // _FORM_WNDOWNERDRAWFRAME_H
