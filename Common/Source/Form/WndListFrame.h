/*
   LK8000 Tactical Flight Computer -  WWW.LK8000.IT
   Released under GNU/GPL License v.2 or later
   See CREDITS.TXT file for authors and copyrights

   $Id: WndListFrame.h,v 1.1 2011/12/21 10:35:29 root Exp root $
*/

#ifndef _FORM_WNDLISTFRAME_H
#define _FORM_WNDLISTFRAME_H

#include "WndFrame.h"

class WndListFrame : public WndFrame {
 public:

  struct ListInfo_t {
    int TopIndex;
    int BottomIndex;
    int ItemIndex;
    int DrawIndex;
    int ScrollIndex;
    int ItemCount;
    int ItemInPageCount;
  };

  using OnListCallback_t = std::function<void(WndListFrame*, ListInfo_t*)>;

  WndListFrame(WindowControl* Owner, TCHAR* Name, int X, int Y, int Width, int Height,
               OnListCallback_t&& OnListCallback);

  virtual bool OnMouseMove(const POINT& Pos);
  bool OnItemKeyDown(WindowControl* Sender, unsigned KeyCode);
  int PrepareItemDraw();
  void ResetList();

  void SetEnterCallback(OnListCallback_t&& OnListCallback) { mOnListEnterCallback = std::move(OnListCallback); }

  void RedrawScrolled(bool all);
  bool RecalculateIndices(bool bigscroll);
  void Redraw();

  int GetItemIndex() { return (mListInfo.ScrollIndex + mListInfo.ItemIndex); }

  int GetDrawIndex() { return (mListInfo.ScrollIndex + mListInfo.DrawIndex); }

  void SetItemIndexPos(int iValue);

  void SetItemIndex(int iValue);
  void SelectItemFromScreen(int xPos, int yPos, RECT* rect, bool select);

  void CalcChildRect(int& x, int& y, int& cx, int& cy) const;

 protected:
  int GetScrollBarWidth();
  int GetScrollBarHeight();

  int GetScrollBarTop() { return GetScrollBarWidth(); }

  int GetScrollIndexFromScrollBarTop(int iScrollBarTop);
  int GetScrollBarTopFromScrollIndex();

  void DrawScrollBar(LKSurface& Surface);

  virtual void Paint(LKSurface& Surface);

#ifdef WIN32
  bool OnLButtonDownNotify(Window* pWnd, const POINT& Pos) override;
#endif

  virtual bool OnLButtonDown(const POINT& Pos);
  virtual bool OnLButtonUp(const POINT& Pos);

 private:
  constexpr static int ScrollbarWidthInitial = 32;
  int ScrollbarWidth;

  OnListCallback_t mOnListCallback;
  OnListCallback_t mOnListEnterCallback;
  ListInfo_t mListInfo;

  RECT rcScrollBarButton;
  RECT rcScrollBar;
  POINT mScrollStart;
  int mMouseScrollBarYOffset;  // where in the scrollbar button was mouse down at
  bool mMouseDown;
  bool mCaptureScrollButton;  // scrolling using scrollbar in progress
  bool mCaptureScroll;        // "Smartphone like" scrolling in progress
};

#endif // _FORM_WNDLISTFRAME_H
