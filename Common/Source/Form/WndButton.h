/*
   LK8000 Tactical Flight Computer -  WWW.LK8000.IT
   Released under GNU/GPL License v.2 or later
   See CREDITS.TXT file for authors and copyrights

   $Id: WndButton.h,v 1.1 2011/12/21 10:35:29 root Exp root $
*/

#ifndef _FORM_WNDBUTTON_H
#define _FORM_WNDBUTTON_H

#include "WindowControls.h"

#define LEDMODE_DISABLED    0
#define LEDMODE_REDGREEN    1
#define LEDMODE_OFFGREEN    2
#define LEDMODE_MANUAL      3

#define LEDCOLOR_BLACK    0U
#define LEDCOLOR_RED      1U
#define LEDCOLOR_GREEN    2U
#define LEDCOLOR_BLUE     3U
#define LEDCOLOR_YELLOW   4U
#define LEDCOLOR_ORANGE   5U
#define LEDCOLOR_LGREEN   6U
#define LEDCOLOR_DGREEN   7U
#define LEDCOLOR_CYAN     8U
#define MAXLEDCOLORS      9U

class WndButton: public WindowControl {
  public:
    using ClickNotifyCallback_t = std::function<void(WndButton*)>;

  protected:
    void DrawPushButton(LKSurface& Surface);

  private:
    void Paint(LKSurface& Surface) override;
    bool mDown;
    unsigned short mLedMode;
    bool mLedOnOff;
    unsigned short mLedSize;
    unsigned short mLedColor;
    int mLastDrawTextHeight;
    ClickNotifyCallback_t mOnClickNotify;

  public:
    WndButton(WindowControl *Parent, const TCHAR *Name, const TCHAR *Caption, int X, int Y, int Width, int Height, ClickNotifyCallback_t&& Function);

    bool OnLButtonDown(const POINT& Pos) override;
    bool OnLButtonUp(const POINT& Pos) override;
    bool OnLButtonDblClick(const POINT& Pos) override;
    bool OnKeyDown(unsigned KeyCode) override;
    bool OnKeyUp(unsigned KeyCode) override;

    virtual void LedSetMode(unsigned short leduse);
    virtual void LedSetSize(unsigned short ledsize);
    virtual void LedSetColor(unsigned short ledcolor);
    virtual void LedSetOnOff(bool ledonoff);

    void SetOnClickNotify(ClickNotifyCallback_t&& Function) {
      mOnClickNotify = std::move(Function);
    }
};

#endif // _FORM_WNDBUTTON_H
