/*
   LK8000 Tactical Flight Computer -  WWW.LK8000.IT
   Released under GNU/GPL License v.2 or later
   See CREDITS.TXT file for authors and copyrights

   $Id: WndListFrame.cpp,v 1.1 2023/10/21 10:35:29 root Exp root $
*/

#include "WndListFrame.h"
#include "MathFunctions.h"
#include "RGB.h"
#include "Event/Event.h"
#include "Screen/LKBitmapSurface.h"
#include "ScreenGeometry.h"
#include "Util/Clamp.hpp"
#include "Bitmaps.h"
#ifndef USE_GDI
#include "Screen/SubCanvas.hpp"
#endif

WndListFrame::WndListFrame(WindowControl* Owner, TCHAR* Name, int X, int Y, int Width, int Height,
                           OnListCallback_t&& OnListCallback)
    : WndFrame(Owner, Name, X, Y, Width, Height) {
  mListInfo.ScrollIndex = 0;
  mListInfo.ItemIndex = 0;
  mListInfo.DrawIndex = 0;
  mListInfo.ItemInPageCount = 0;
  mListInfo.TopIndex = 0;
  mListInfo.BottomIndex = 0;
  mListInfo.ItemCount = 0;

  mOnListCallback = OnListCallback;
  mOnListEnterCallback = NULL;
  SetForeColor(RGB_LISTFG);
  SetBackColor(RGB_LISTBG);
  mMouseDown = false;
  mCaptureScrollButton = false;
  ScrollbarWidth = -1;

  rcScrollBarButton.top =
      0;  // make sure this rect is initialized so we don't "loose" random lbuttondown events if scrollbar not drawn
  rcScrollBarButton.bottom = 0;
  rcScrollBarButton.left = 0;
  rcScrollBarButton.right = 0;

  rcScrollBar.left = 0;  // don't need to initialize this rect, but it's good practice
  rcScrollBar.right = 0;
  rcScrollBar.top = 0;
  rcScrollBar.bottom = 0;
};

void WndListFrame::CalcChildRect(int& x, int& y, int& cx, int& cy) const {
  if (cx < 0) {
    cx -= const_cast<WndListFrame*>(this)->GetScrollBarWidth();
  }
  WndFrame::CalcChildRect(x, y, cx, cy);
}

void WndListFrame::Paint(LKSurface& Surface) {
  WndFrame* pChildFrame = NULL;
  if (!mClients.empty()) {
    pChildFrame = (WndFrame*)mClients.front();
    pChildFrame->SetIsListItem(true);
  }

  WndFrame::Paint(Surface);

  if (pChildFrame) {
#ifdef USE_GDI
    LKBitmapSurface TmpSurface;
    TmpSurface.Create(Surface, pChildFrame->GetWidth(), pChildFrame->GetHeight());
    const auto oldFont = TmpSurface.SelectObject(pChildFrame->GetFont());
#endif

    for (int i = 0; i < mListInfo.ItemInPageCount; i++) {
      if (mOnListCallback != NULL) {
        mListInfo.DrawIndex = mListInfo.TopIndex + i;
        if (mListInfo.DrawIndex == mListInfo.ItemIndex)
          continue;
        mOnListCallback(this, &mListInfo);
      }

      const RasterPoint offset(pChildFrame->GetLeft(),
                               i * pChildFrame->GetHeight() + ((GetBorderKind() & BORDERTOP) ? DLGSCALE(1) : 0));
      const PixelSize size(pChildFrame->GetWidth(), pChildFrame->GetHeight());

#ifndef USE_GDI

      SubCanvas TmpCanvas(Surface, offset, size);
      LKSurface TmpSurface;
      TmpSurface.Attach(&TmpCanvas);
      TmpSurface.SelectObject(pChildFrame->GetFont());
#endif

      pChildFrame->PaintSelector(true);
      pChildFrame->Paint(TmpSurface);
      pChildFrame->PaintSelector(false);

#ifdef USE_GDI
      Surface.Copy(offset.x, offset.y, size.cx, size.cy, TmpSurface, 0, 0);
    }
    TmpSurface.SelectObject(oldFont);
#else
    }
#endif

    mListInfo.DrawIndex = mListInfo.ItemIndex;

    DrawScrollBar(Surface);
  }
}

void WndListFrame::Redraw() {
  WindowControl::Redraw();  // redraw all but not the current
  if (!mClients.empty()) {
    mClients.front()->Redraw();  // redraw the current
  }
}

int WndListFrame::GetScrollBarWidth() {
  if (ScrollbarWidth == -1) {
    // use fat Scroll on Embedded platform for allow usage on touch screen with fat finger ;-)
    constexpr double scale = HasTouchScreen() ? 1.0 : 0.75;
    ScrollbarWidth = DLGSCALE(ScrollbarWidthInitial * scale);
  }
  return ScrollbarWidth;
}

void WndListFrame::DrawScrollBar(LKSurface& Surface) {
  if (mListInfo.BottomIndex == mListInfo.ItemCount) {  // don't need scroll bar if one page only
    return;
  }

  // ENTIRE SCROLLBAR AREA
  // also used for mouse events
  rcScrollBar = {(int)(GetWidth() - GetScrollBarWidth()), 0, (int)GetWidth(), (int)GetHeight()};

  const auto oldPen = Surface.SelectObject(LKPen_Black_N1);
  const auto oldBrush = Surface.SelectObject(LK_HOLLOW_BRUSH);

  // draw rectangle around entire scrollbar area
  Surface.Rectangle(rcScrollBar.left, rcScrollBar.top, rcScrollBar.right, rcScrollBar.bottom);

  // Just Scroll Bar Slider button
  rcScrollBarButton = {
      rcScrollBar.left, GetScrollBarTopFromScrollIndex(), rcScrollBar.right,
      GetScrollBarTopFromScrollIndex() + GetScrollBarHeight() + 2,  // +2 for 3x pen, +1 for 2x pen
  };

  // TOP Dn Button 32x32
  // BOT Up Button 32x32
  hScrollBarBitmapTop.Draw(Surface, rcScrollBar.left, rcScrollBar.top, ScrollbarWidth, ScrollbarWidth);
  hScrollBarBitmapBot.Draw(Surface, rcScrollBar.left, rcScrollBar.bottom - ScrollbarWidth, ScrollbarWidth,
                           ScrollbarWidth);

  if (mListInfo.ItemCount > mListInfo.ItemInPageCount) {
    // Middle Slider Button 30x28
    // handle on slider
    const int SCButtonW = _MulDiv(30, ScrollbarWidth, ScrollbarWidthInitial);
    const int SCButtonH = _MulDiv(28, ScrollbarWidth, ScrollbarWidthInitial);
    const int SCButtonY = _MulDiv(14, ScrollbarWidth, ScrollbarWidthInitial);
    hScrollBarBitmapMid.Draw(Surface, rcScrollBar.left, rcScrollBarButton.top + GetScrollBarHeight() / 2 - SCButtonY,
                             SCButtonW, SCButtonH);

    // box around slider rect
    Surface.SelectObject(LKPen_Black_N2);
    // just left line of scrollbar
    Surface.Rectangle(rcScrollBarButton.left, rcScrollBarButton.top, rcScrollBarButton.right, rcScrollBarButton.bottom);

  }  // more items than fit on screen

  Surface.SelectObject(oldPen);
  Surface.SelectObject(oldBrush);
}

void WndListFrame::RedrawScrolled(bool all) {
  mListInfo.DrawIndex = mListInfo.ItemIndex;
  mOnListCallback(this, &mListInfo);
  WindowControl* pChild = mClients.front();
  if (pChild) {
    const int newTop = pChild->GetHeight() * (mListInfo.ItemIndex - mListInfo.TopIndex) +
                       ((GetBorderKind() & BORDERTOP) ? DLGSCALE(1) : 0);
    if (newTop == pChild->GetTop()) {
      Redraw();  // non moving the helper window force redraw
    } else {
      pChild->SetTop(newTop);  // moving the helper window invalidate the list window
      pChild->Redraw();
      // to be optimized: after SetTop Paint redraw all list items
    }
  }
}

bool WndListFrame::RecalculateIndices(bool bigscroll) {
  // scroll to smaller of current scroll or to last page
  mListInfo.ScrollIndex = std::max(0, std::min(mListInfo.ScrollIndex, mListInfo.ItemCount - mListInfo.ItemInPageCount));

  // if we're off end of list, move scroll to last page and select 1st item in last page, return
  if (mListInfo.ItemIndex + mListInfo.ScrollIndex >= mListInfo.ItemCount) {
    mListInfo.ItemIndex = std::max(0, mListInfo.ItemCount - mListInfo.ScrollIndex - 1);
    mListInfo.ScrollIndex = std::max(0, std::min(mListInfo.ScrollIndex, mListInfo.ItemCount - mListInfo.ItemIndex - 1));
    return false;
  }

  // again, check to see if we're too far off end of list
  mListInfo.ScrollIndex = std::max(0, std::min(mListInfo.ScrollIndex, mListInfo.ItemCount - mListInfo.ItemIndex - 1));

  if (mListInfo.ItemIndex >= mListInfo.BottomIndex) {
    if ((mListInfo.ItemCount > mListInfo.ItemInPageCount) &&
        (mListInfo.ItemIndex + mListInfo.ScrollIndex < mListInfo.ItemCount)) {
      mListInfo.ScrollIndex++;
      mListInfo.ItemIndex = mListInfo.BottomIndex - 1;
      // JMW scroll
      RedrawScrolled(true);
      return true;
    } else {
      mListInfo.ItemIndex = mListInfo.BottomIndex - 1;
      return false;
    }
  }
  if (mListInfo.ItemIndex < 0) {
    mListInfo.ItemIndex = 0;
    // JMW scroll
    if (mListInfo.ScrollIndex > 0) {
      mListInfo.ScrollIndex--;
      RedrawScrolled(true);
      return true;
    } else {
      // only return if no more scrolling left to do
      return false;
    }
  }
  RedrawScrolled(bigscroll);
  return true;
}

bool WndListFrame::OnItemKeyDown(WindowControl* Sender, unsigned KeyCode) {
  switch (KeyCode) {
    case KEY_RETURN:
      if (mOnListEnterCallback) {
        mOnListEnterCallback(this, &mListInfo);
        RedrawScrolled(false);
        return true;
      }
      return false;
    case KEY_LEFT:
      if ((mListInfo.ScrollIndex > 0) && (mListInfo.ItemCount > mListInfo.ItemInPageCount)) {
        mListInfo.ScrollIndex -= mListInfo.ItemInPageCount;
      }
      return RecalculateIndices(true);
    case KEY_RIGHT:
      if ((mListInfo.ItemIndex + mListInfo.ScrollIndex < mListInfo.ItemCount) &&
          (mListInfo.ItemCount > mListInfo.ItemInPageCount)) {
        mListInfo.ScrollIndex += mListInfo.ItemInPageCount;
      }
      return RecalculateIndices(true);
    case KEY_DOWN:
      mListInfo.ItemIndex++;
      return RecalculateIndices(false);
    case KEY_UP:
      mListInfo.ItemIndex--;
      return RecalculateIndices(false);
  }
  mMouseDown = false;
  return false;
}

void WndListFrame::ResetList() {
  mListInfo.ScrollIndex = 0;
  mListInfo.ItemIndex = 0;
  mListInfo.DrawIndex = 0;

  WindowControl* pChild = mClients.front();

  mListInfo.ItemInPageCount = (pChild ? (((GetHeight() + pChild->GetHeight() - 1) / pChild->GetHeight()) - 1) : 0);
  mListInfo.TopIndex = 0;
  mListInfo.BottomIndex = 0;
  mListInfo.ItemCount = 0;

  if (mOnListCallback != NULL) {
    mListInfo.DrawIndex = -1;  // -1 -> initialize data
    mOnListCallback(this, &mListInfo);
    mListInfo.DrawIndex = 0;  // setup data for first item,
    mOnListCallback(this, &mListInfo);
  }

  if (mListInfo.BottomIndex == 0) {  // calc bounds
    mListInfo.BottomIndex = mListInfo.ItemCount;
    if (mListInfo.BottomIndex > mListInfo.ItemInPageCount) {
      mListInfo.BottomIndex = mListInfo.ItemInPageCount;
    }
  }

  if (pChild) {
    pChild->SetTop(0);  // move item window to the top
    pChild->Redraw();
  }
}

int WndListFrame::PrepareItemDraw() {
  if (mOnListCallback)
    mOnListCallback(this, &mListInfo);
  return (1);
}

void WndListFrame::SetItemIndexPos(int iValue) {
  int Total = mListInfo.ItemCount;
  mListInfo.ScrollIndex = 0;
  mListInfo.ItemIndex = iValue;

  if (Total > mListInfo.ItemInPageCount) {
    if (iValue > (mListInfo.ItemInPageCount - 1)) {
      mListInfo.ScrollIndex = iValue - mListInfo.ItemInPageCount;
      mListInfo.ItemIndex = iValue - mListInfo.ScrollIndex;
    }

    if ((Total - iValue) < (mListInfo.ItemInPageCount)) {
      mListInfo.ScrollIndex = Total - mListInfo.ItemInPageCount;
      mListInfo.ItemIndex = iValue - mListInfo.ScrollIndex;
    }
  }
  RecalculateIndices(false);
}

void WndListFrame::SetItemIndex(int iValue) {
  mListInfo.ItemIndex = 0;  // usually leaves selected item as first in screen
  mListInfo.ScrollIndex = iValue;

  int iTail = mListInfo.ItemCount - iValue;  // if within 1 page of end
  if (iTail < mListInfo.ItemInPageCount) {
    int iDiff = mListInfo.ItemInPageCount - iTail;
    int iShrinkBy = std::min(iValue, iDiff);  // don't reduce by

    mListInfo.ItemIndex += iShrinkBy;
    mListInfo.ScrollIndex -= iShrinkBy;
  }

  RecalculateIndices(false);
}

void WndListFrame::SelectItemFromScreen(int xPos, int yPos, RECT* rect, bool select) {
  if (PtInRect(&rcScrollBar, {xPos, yPos})) {
    // ignore if click inside scrollbar
    return;
  }

  WindowControl* pChild = NULL;
  if (!mClients.empty()) {
    pChild = mClients.front();

    *rect = GetClientRect();
    int index = yPos / pChild->GetHeight();  // yPos is offset within ListEntry item!

    if ((index >= 0) && (index < mListInfo.BottomIndex)) {
      if (mListInfo.ItemIndex != index) {
        mListInfo.ItemIndex = index;
        RecalculateIndices(false);
      } else {
        if (!select) {
          if (mOnListEnterCallback) {
            mOnListEnterCallback(this, &mListInfo);
          }
          RedrawScrolled(false);
        }
      }
    }
  }
}

bool WndListFrame::OnMouseMove(const POINT& Pos) {
  if (mMouseDown) {
    SetCapture();

    if (mCaptureScrollButton) {
      const int iScrollBarTop = std::max(1, (int)Pos.y - mMouseScrollBarYOffset);
      const int iScrollIndex = GetScrollIndexFromScrollBarTop(iScrollBarTop);

      if (iScrollIndex != mListInfo.ScrollIndex) {
        const int iScrollAmount = iScrollIndex - mListInfo.ScrollIndex;
        mListInfo.ScrollIndex = mListInfo.ScrollIndex + iScrollAmount;
        Redraw();
      }
    } else if (mListInfo.ItemCount > mListInfo.ItemInPageCount) {
      const int ScrollOffset = Pos.y - mScrollStart.y;
      const int ScrollStep = GetHeight() / mListInfo.ItemInPageCount;
      const int newIndex = Clamp(mListInfo.ScrollIndex - (ScrollOffset / ScrollStep), 0,
                                 mListInfo.ItemCount - mListInfo.ItemInPageCount);
      if (newIndex != mListInfo.ScrollIndex) {
        mListInfo.ScrollIndex = newIndex;
        mScrollStart = Pos;
        mCaptureScroll = true;
      }
      Redraw();
    }
  }
  return false;
}

#ifdef WIN32
bool WndListFrame::OnLButtonDownNotify(Window* pWnd, const POINT& Pos) {
  POINT NewPos(Pos);
  pWnd->ToScreen(NewPos);
  ToClient(NewPos);

  return OnLButtonDown(NewPos);
}
#endif

bool WndListFrame::OnLButtonDown(const POINT& Pos) {
  mMouseDown = true;
  mCaptureScroll = false;
  SetCapture();
  if (PtInRect(&rcScrollBarButton, Pos))  // see if click is on scrollbar handle
  {
    mMouseScrollBarYOffset = std::max(0, (int)Pos.y - (int)rcScrollBarButton.top);  // start mouse drag
    mCaptureScrollButton = true;

  } else if (PtInRect(&rcScrollBar, Pos)) {  // clicked in scroll bar up/down/pgup/pgdn

    if (Pos.y - rcScrollBar.top < (ScrollbarWidth)) {  // up arrow
      mListInfo.ScrollIndex = std::max(0, mListInfo.ScrollIndex - 1);
    } else if (rcScrollBar.bottom - Pos.y < (ScrollbarWidth)) {  // down arrow
      mListInfo.ScrollIndex = std::max(0, std::min(mListInfo.ItemCount - mListInfo.ItemInPageCount, mListInfo.ScrollIndex + 1));
    } else if (Pos.y < rcScrollBarButton.top) {  // page up
      mListInfo.ScrollIndex = std::max(0, mListInfo.ScrollIndex - mListInfo.ItemInPageCount);
    } else if (mListInfo.ItemCount > mListInfo.ScrollIndex + mListInfo.ItemInPageCount) {  // page down
      mListInfo.ScrollIndex =
          std::min(mListInfo.ItemCount - mListInfo.ItemInPageCount, mListInfo.ScrollIndex + mListInfo.ItemInPageCount);
    }
    Redraw();
  } else {
    mScrollStart = Pos;
  }
  return true;
}

bool WndListFrame::OnLButtonUp(const POINT& Pos) {
  ReleaseCapture();

  if (mMouseDown) {
    mMouseDown = false;
    if (!mCaptureScrollButton) {
      if (!mClients.empty()) {
        RECT Rc = {};
        SelectItemFromScreen(Pos.x, Pos.y, &Rc, mCaptureScroll);
        mClients.front()->SetFocus();
      }
    }
  }
  mCaptureScroll = false;
  mCaptureScrollButton = false;
  return true;
}

inline int WndListFrame::GetScrollBarHeight() {
  const int h = GetHeight() - 2 * GetScrollBarTop();
  if (mListInfo.ItemCount == 0) {
    return h;
  } else {
    return std::max(ScrollbarWidth, _MulDiv(h, mListInfo.ItemInPageCount, mListInfo.ItemCount));
  }
}

inline int WndListFrame::GetScrollIndexFromScrollBarTop(int iScrollBarTop) {
  const int h = GetHeight() - 2 * GetScrollBarTop();
  if ((h - GetScrollBarHeight()) == 0) {
    return 0;
  }

  // Number of item excluding first Page
  const int NbHidden = mListInfo.ItemCount - mListInfo.ItemInPageCount;

  return std::max(
      0,
      std::min(NbHidden, std::max(0, ((NbHidden * (iScrollBarTop - GetScrollBarTop())) / (h - GetScrollBarHeight())))));
}

inline int WndListFrame::GetScrollBarTopFromScrollIndex() {
  int iRetVal = 0;
  int h = GetHeight() - 2 * GetScrollBarTop();
  if (mListInfo.ItemCount - mListInfo.ItemInPageCount == 0) {
    iRetVal = h;
  } else {
    iRetVal = (GetScrollBarTop() + (mListInfo.ScrollIndex) * (h - GetScrollBarHeight()) /
                                       (mListInfo.ItemCount - mListInfo.ItemInPageCount));
  }
  return iRetVal;
}
