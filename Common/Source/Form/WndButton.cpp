/*
   LK8000 Tactical Flight Computer -  WWW.LK8000.IT
   Released under GNU/GPL License v.2 or later
   See CREDITS.TXT file for authors and copyrights

   $Id: WndButton.cpp,v 8.10 2010/12/13 01:17:08 root Exp root $
*/

#include "externs.h"
#include "WndButton.h"
#include "RGB.h"
#include "Event/Key.h"
#include "Asset.hpp"
#include "Sideview.h"

//
// Led button color ramps
//
//
// blending from rgb1 to rgb2 colors, bottom->up
//
struct LEDCOLORRAMP {
  LKColor color1;
  LKColor color2;
  unsigned short l;
};

constexpr LEDCOLORRAMP LedRamp[MAXLEDCOLORS]= {
     { {   0,   0,   0 }, { 140,  140,  140 }, 10 },  // black
     { { 160,   0,   0 }, { 255,  130,  130 }, 10 },  // red ok
     { {   0, 123,   0 }, {  80,  255,   80 }, 10 },  // green ok
     { {   0,   0, 160 }, { 120,  120,  255 }, 10 },  // blue ok
     { { 130, 130,   0 }, { 255,  255,    0 }, 10 },  // yellow ok
     { { 255, 128,   0 }, { 255,  185,  140 }, 10 },  // orange ok
     { { 164, 255, 164 }, { 255,  255,  255 },  5 },  // light green for dithering
     { {   0, 0,   0   }, {  70,  140,   70 }, 10 },  // very dark green
     { {   0,  90,  90 }, {   0,  255,  255 }, 10 }   // cyan
};

//
// Distance from button borders for drawing the led rectangle
//
#define LEDBUTTONBORDER 3


//-----------------------------------------------------------
// WndButton
//-----------------------------------------------------------

WndButton::WndButton(WindowControl* Parent, const TCHAR* Name, const TCHAR* Caption, int X, int Y, int Width,
                     int Height, ClickNotifyCallback_t&& Function)
    : WindowControl(Parent, Name, X, Y, Width, Height), mOnClickNotify(std::move(Function)) {
  mDown = false;
  mCanFocus = true;

  mLedMode = LEDMODE_DISABLED;
  mLedOnOff = false;
  mLedSize = 5;  // the default size, including pen borders
  mLedColor = LEDCOLOR_BLACK;

  SetForeColor(RGB_BUTTONFG);
  SetBackColor(GetParent()->GetBackColor());

  if (Caption) {
    SetCaption(Caption);
  }
  mLastDrawTextHeight = -1;
}

void WndButton::LedSetMode(unsigned short ledmode) {
  mLedMode = ledmode;
}

void WndButton::LedSetSize(unsigned short ledsize) {
  LKASSERT(ledsize > 0 && ledsize < 200);
  mLedSize = ledsize;
}

void WndButton::LedSetOnOff(bool ledonoff) {
  mLedOnOff = ledonoff;
}

void WndButton::LedSetColor(unsigned short ledcolor) {
  LKASSERT(ledcolor < MAXLEDCOLORS);
  mLedColor = ledcolor;
}

bool WndButton::OnKeyDown(unsigned KeyCode) {
  switch (KeyCode) {
    case KEY_RETURN:
    case KEY_SPACE:
      if (!mDown) {
        mDown = true;
        Redraw();
        return true;
      }
  }
  return false;
}

bool WndButton::OnKeyUp(unsigned KeyCode) {
  switch (KeyCode) {
    case KEY_RETURN:
    case KEY_SPACE:
      if (!Debounce())
        return (1);  // prevent false trigger
      if (mDown) {
        mDown = false;
        Redraw();
        if (mOnClickNotify != NULL) {
          (mOnClickNotify)(this);
          return true;
        }
      }
  }
  return false;
}

bool WndButton::OnLButtonDown(const POINT& Pos) {
  mDown = true;
  SetCapture();
  if (!HasFocus()) {
    SetFocus();
  } else {
    Redraw();
  }
  return true;
}

bool WndButton::OnLButtonUp(const POINT& Pos) {
  bool bTmp = mDown;
  mDown = false;
  ReleaseCapture();
  Redraw();
  if (bTmp && mOnClickNotify != NULL) {
    RECT rcClient = GetClientRect();
    if (PtInRect(&rcClient, Pos)) {
      (mOnClickNotify)(this);
    }
  }
  return true;
}

bool WndButton::OnLButtonDblClick(const POINT& Pos) {
  mDown = true;
  Redraw();
  return true;
}

void WndButton::DrawPushButton(LKSurface& Surface) {
  PixelRect rc(GetClientRect());
  rc.Grow(-2);  // todo border width
  Surface.DrawPushButton(rc, mDown);
}

void WndButton::Paint(LKSurface& Surface) {
  if (!IsVisible())
    return;

  WindowControl::Paint(Surface);

  const PixelRect rcClient(GetClientRect());

  DrawPushButton(Surface);

  if (mLedMode) {
    PixelRect rc = rcClient;
    InflateRect(&rc, -2, -2);  // todo border width

    RECT lrc = {rc.left + (LEDBUTTONBORDER - 1), rc.bottom - (LEDBUTTONBORDER + DLGSCALE(mLedSize)),
                rc.right - LEDBUTTONBORDER, rc.bottom - LEDBUTTONBORDER};
    unsigned lcol = 0;
    switch (mLedMode) {
      case LEDMODE_REDGREEN:
        if (IsDithered())
          lcol = (mLedOnOff ? LEDCOLOR_LGREEN : LEDCOLOR_ORANGE);
        else {
          if (HasFocus())
            lcol = LEDCOLOR_YELLOW;
          else
            lcol = (mLedOnOff ? LEDCOLOR_GREEN : LEDCOLOR_RED);
        }
        break;
      case LEDMODE_OFFGREEN:
        if (IsDithered())
          lcol = (mLedOnOff ? LEDCOLOR_LGREEN : LEDCOLOR_BLUE);
        else {
          if (HasFocus())
            lcol = LEDCOLOR_YELLOW;
          else
            lcol = (mLedOnOff ? LEDCOLOR_GREEN : LEDCOLOR_DGREEN);
        }
        break;
      case LEDMODE_MANUAL:
        lcol = mLedColor;
        break;
      default:
        LKASSERT(0);
        break;
    }
    RenderSky(Surface, lrc, LedRamp[lcol].color1, LedRamp[lcol].color2, LedRamp[lcol].l);
  }

  const TCHAR* szCaption = GetWndText();
  const size_t nSize = _tcslen(szCaption);
  if (nSize > 0) {
    Surface.SetTextColor(IsDithered() ? (mDown ? RGB_WHITE : RGB_BLACK) : GetForeColor());

    Surface.SetBkColor(IsDithered() ? (mDown ? RGB_BLACK : RGB_WHITE) : GetBackColor());

    Surface.SetBackgroundTransparent();

    const auto oldFont = Surface.SelectObject(GetFont());
    PixelRect rc = rcClient;
    InflateRect(&rc, -2, -2);  // todo border width

    if (mDown)
      OffsetRect(&rc, 2, 2);

    if (mLastDrawTextHeight < 0) {
      Surface.DrawText(
          szCaption, &rc,
          DT_CALCRECT | DT_EXPANDTABS | DT_CENTER | DT_NOCLIP | DT_WORDBREAK  // mCaptionStyle // | DT_CALCRECT
      );

      if (!rcClient.IsInside(rc.GetTopLeft()) || !rcClient.IsInside(rc.GetBottomRight())) {
        TestLog(_T("Warning : Text too long <%s>"), szCaption);
      }

      mLastDrawTextHeight = rc.bottom - rc.top;
      // DoTo optimize
      rc = rcClient;
      InflateRect(&rc, -2, -2);  // todo border width
      if (mDown)
        OffsetRect(&rc, 2, 2);
    }

    unsigned height = GetHeight();
    unsigned offset = ((GetHeight() - 4 - mLastDrawTextHeight) / 2);
    if (offset < height) {
      rc.top += offset;
    }

    Surface.DrawText(szCaption, &rc,
                     DT_EXPANDTABS | DT_CENTER | DT_NOCLIP | DT_WORDBREAK  // mCaptionStyle // | DT_CALCRECT
    );

    Surface.SelectObject(oldFont);
  }
}
