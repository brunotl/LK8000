/*
   LK8000 Tactical Flight Computer -  WWW.LK8000.IT
   Released under GNU/GPL License v.2 or later
   See CREDITS.TXT file for authors and copyrights

   $Id: WindowControls.h,v 1.1 2011/12/21 10:35:29 root Exp root $
*/

#ifndef _FORM_WINDOWSCONTROL_H
#define _FORM_WINDOWSCONTROL_H

#include "Screen/LKWindowSurface.h"
#include "Screen/BrushReference.h"
#include "Screen/PenReference.h"
#include "Screen/FontReference.h"
#include "Window/WndCtrlBase.h"
#include "LKObjects.h"
#include <tchar.h>
#include <string.h>
#include <functional>
#include <list>

#include "Time/PeriodClock.hpp"

#define BORDERTOP (1 << bkTop)
#define BORDERRIGHT (1 << bkRight)
#define BORDERBOTTOM (1 << bkBottom)
#define BORDERLEFT (1 << bkLeft)

class WndProperty;
class WndForm;  // Forward declaration

enum BorderKind_t { 
  bkNone,
  bkTop,
  bkRight,
  bkBottom,
  bkLeft
};

class WindowControl : public WndCtrlBase {
  friend class WndForm;
  friend class WndProperty;

 private:
  WindowControl* mOwner;
  WndForm* mParentWndForm;

  int mBorderKind;

  LKColor mColorBack;
  LKColor mColorFore;
  LKBrush mBrushBk;
  LKBrush mBrushBorder;

  int mTag;
  bool mReadOnly;

  static int InstCount;

 protected:
  bool mCanFocus;
  bool mDontPaintSelector;

  std::list<WindowControl*> mClients;

  virtual void PaintBorder(LKSurface& Surface);
  virtual void PaintSelector(LKSurface& Surface);

  bool OnPaint(LKSurface& Surface, const RECT& Rect) override;

 public:
  const TCHAR* GetCaption() const { return GetWndText(); }

  virtual void CalcChildRect(int& x, int& y, int& cx, int& cy) const;

  // only Call by final contructor or overwrite
  virtual void AddClient(WindowControl* Client);

  virtual void Paint(LKSurface& Surface);

  WindowControl* GetCanFocus();
  bool SetCanFocus(bool Value);

  bool GetReadOnly() { return mReadOnly; };
  virtual bool SetReadOnly(bool Value);

  int GetBorderKind();
  virtual int SetBorderKind(int Value);

  virtual LKColor SetForeColor(const LKColor& Value);
  const LKColor& GetForeColor() const { return mColorFore; };

  virtual LKColor SetBackColor(const LKColor& Value);
  const LKColor& GetBackColor() const { return mColorBack; };

  BrushReference GetBackBrush() const {
    if (mBrushBk) {
      return mBrushBk;
    } else if (GetParent()) {
      return GetParent()->GetBackBrush();
    } else {
      return LKBrush_FormBackGround;
    }
  }

  void SetBorderColor(const LKColor& Color) { mBrushBorder.Create(Color); }

  virtual void SetCaption(const TCHAR* Value);

  virtual WindowControl* GetClientArea() { return this; }

  virtual WindowControl* GetParent() const { return mOwner; };
  virtual WndForm* GetParentWndForm() { return mParentWndForm; }

  int GetTag() { return mTag; }

  int SetTag(int Value) {
    mTag = Value;
    return mTag;
  }

  void SetTop(int Value);
  void SetLeft(int Value);
  void SetWidth(unsigned int Value);
  void SetHeight(unsigned int Value);

  WindowControl* FocusNext(WindowControl* Sender);
  WindowControl* FocusPrev(WindowControl* Sender);

  WindowControl(WindowControl* Owner, const TCHAR* Name, int X, int Y, int Width, int Height, bool Visible = true);
  ~WindowControl();

  void Destroy() override;

  void PaintSelector(bool Value) { mDontPaintSelector = Value; }

  template <typename TypeT = WindowControl>
  TypeT* FindByName(const TCHAR* Name) {
    static_assert(std::is_base_of_v<WindowControl, TypeT>,
                  "template parameters must have `WindowControl` as base class");
    WindowControl* pWnd = FindChild(Name);
    auto pTypeT = dynamic_cast<TypeT*>(pWnd);
    assert(pWnd == pTypeT);  // ctrl found is not an instance of the requested class
    return pTypeT;
  }

  void ForEachChild(std::function<void(WindowControl*)> visitor);

 protected:
  WindowControl* FindChild(const TCHAR* Name);

  void OnSetFocus() override;
  void OnKillFocus() override;

  bool OnClose() override {
    SetVisible(false);
    return true;
  }
};

extern WindowControl* ActiveControl;
extern WindowControl* LastFocusControl;

#endif // _FORM_WINDOWSCONTROL_H
