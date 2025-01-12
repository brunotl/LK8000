/*
   LK8000 Tactical Flight Computer -  WWW.LK8000.IT
   Released under GNU/GPL License v.2 or later
   See CREDITS.TXT file for authors and copyrights

   $Id: dlgComboPicker.cpp,v 1.1 2011/12/21 10:29:29 root Exp root $
*/

#include "externs.h"
#include "InputEvents.h"
#include "Dialogs.h"
#include "dlgTools.h"
#include "resource.h"


WndProperty * wComboPopupWndProperty;
std::weak_ptr<DataField> ComboPopupDataFieldWeakPtr;
ComboList * ComboListPopup=NULL;

static TCHAR sSavedInitialValue[ComboList::ITEMMAX];
static int iSavedInitialDataIndex=-1;

static void OnPaintComboPopupListItem(WndOwnerDrawFrame * Sender, LKSurface& Surface) {

    if (Sender) {

        if (ComboListPopup->DrawListIndex >= 0 &&
                ComboListPopup->DrawListIndex < ComboListPopup->size()) {

            // Fill Background with Highlight color if Selected Item
            if (!Sender->HasFocus() && ComboListPopup->ItemIndex == ComboListPopup->DrawListIndex) {
                RECT rc = Sender->GetClientRect();
                Surface.FillRect(&rc, LKBrush_Higlighted);
            }

            const int w = Sender->GetWidth();
            const int h = Sender->GetHeight();

            const TCHAR* szText = ComboListPopup->ItemList[ComboListPopup->DrawListIndex].ValueFormatted.c_str();

            Surface.SetBackgroundTransparent();
            Surface.SetTextColor(RGB_BLACK);
            const int xText = DLGSCALE(2);
            const int yText = (h - Surface.GetTextHeight(szText)) / 2;
            Surface.DrawTextClip(xText, yText, szText, w - DLGSCALE(2));
        }
    }
}

static void OnComboPopupListInfo(WndListFrame * Sender, WndListFrame::ListInfo_t *ListInfo)
{ // callback function for the ComboPopup
  if (ListInfo->DrawIndex == -1) { // initialize
    ListInfo->ItemCount = ComboListPopup->size();
    ListInfo->ScrollIndex = 0;
    ListInfo->ItemIndex = ComboListPopup->PropertyDataFieldIndexSaved;
  }
  else {
    ComboListPopup->DrawListIndex = ListInfo->DrawIndex + ListInfo->ScrollIndex;
    ComboListPopup->ItemIndex = ListInfo->ItemIndex + ListInfo->ScrollIndex;
  }
}

static void OnHelpClicked(WindowControl * Sender) {
  auto ptr = ComboPopupDataFieldWeakPtr.lock();
  if (ptr) {
    int idx = ComboListPopup->ItemIndex;  
    if (idx >=0) {
      const auto& Item = ComboListPopup->ItemList[idx];
      ptr->SetFromCombo(Item.DataFieldIndex, Item.Value.c_str());
    }
  }
  wComboPopupWndProperty->OnHelp();
}

static void OnCloseClicked(WndButton* pWnd) {
  if(pWnd) {
    WndForm * pForm = pWnd->GetParentWndForm();
    if(pForm) {
      pForm->SetModalResult(mrOK);
    }
  }
}

static void OnComboPopupListEnter(WindowControl * Sender, WndListFrame::ListInfo_t *ListInfo) {
  // double-click on item -- NOT in callback table because added manually
  if(Sender) {
    WndForm * pForm = Sender->GetParentWndForm();
    if(pForm) {
      pForm->SetModalResult(mrOK);
    }
  }
}

static void OnCancelClicked(WndButton* pWnd){
  ComboListPopup->ItemIndex = -1;
  if(pWnd) {
    WndForm* pForm = pWnd->GetParentWndForm();
    if(pForm) {
      pForm->SetModalResult(mrCancel);
    }
  }
}


static CallBackTableEntry_t CallBackTable[]={
  OnListCallbackEntry(OnComboPopupListInfo),
  OnPaintCallbackEntry(OnPaintComboPopupListItem),
  ClickNotifyCallbackEntry(OnHelpClicked),
  ClickNotifyCallbackEntry(OnCloseClicked),
  ClickNotifyCallbackEntry(OnCancelClicked),
  EndCallBackEntry()
};

int dlgComboPicker(WndProperty* pWnd){

  static bool bInComboPicker=false;
  bool bInitialPage=true;
  bool bOpenCombo=true; // used to exit loop (optionally reruns combo with
                        //lower/higher index of items for int/float

  if (bInComboPicker) // prevents multiple instances
    return 0;
  else
    bInComboPicker=true;

  while (bOpenCombo)
  {
    LKASSERT(pWnd);
    wComboPopupWndProperty = pWnd;

    std::unique_ptr<WndForm> wf(dlgLoadFromXML(CallBackTable, ScreenLandscape ? IDR_XML_COMBOPICKER_L : IDR_XML_COMBOPICKER_P));

    if (!wf) return -1;

    wf->SetCaption(pWnd->GetCaption());


    // allow item to be focused / hightlighted
    WindowControl* wComboPopupListEntry = wf->FindByName(TEXT("frmComboPopupListEntry"));
    if (wComboPopupListEntry) {
      wComboPopupListEntry->SetCanFocus(true);
    }

    auto ptr = wComboPopupWndProperty->GetDataField();
    if (ptr) {
      ComboPopupDataFieldWeakPtr = ptr;
      ptr->CreateComboList();   
      ComboListPopup = ptr->GetCombo();

      auto wComboPopupListFrame = wf->FindByName<WndListFrame>(TEXT("frmComboPopupList"));
      if (wComboPopupListFrame) {
        wComboPopupListFrame->SetBorderKind(BORDERLEFT | BORDERTOP | BORDERRIGHT|BORDERBOTTOM);
        wComboPopupListFrame->SetEnterCallback(OnComboPopupListEnter);
        wComboPopupListFrame->ResetList();
        wComboPopupListFrame->SetItemIndex(ComboListPopup->PropertyDataFieldIndexSaved);
      }
    }

    if (bInitialPage) { // save values for "Cancel" from first page only
      bInitialPage=false;
      iSavedInitialDataIndex=ComboListPopup->ItemList[ComboListPopup->PropertyDataFieldIndexSaved].DataFieldIndex;
      auto ptr = ComboPopupDataFieldWeakPtr.lock();
      if (ptr) {
        ptr->CopyString(sSavedInitialValue,false);
      }
    }

    WindowControl* pBtHelp = wf->FindByName(TEXT("cmdHelp"));
    if(pBtHelp) {
       pBtHelp->SetVisible(wComboPopupWndProperty->HasHelpText());
    }

    wf->ShowModal();

    bOpenCombo=false;  //tell  combo to exit loop after close

    DataFieldPtr ComboPopupDataField = ComboPopupDataFieldWeakPtr.lock();
    if (ComboPopupDataField) {
      if (ComboListPopup->ItemIndex >= 0) { // OK/Select

        const auto& item = ComboListPopup->ItemList[ComboListPopup->ItemIndex];

        if (item.DataFieldIndex == ComboList::ReopenMOREDataIndex) {
          // we're last in list and the want more past end of list so select last
          // real list item and reopen
          ComboPopupDataField->SetDetachGUI(true);  // we'll reopen, so don't call xcsoar data changed routine yet
          ComboListPopup->ItemIndex--;
          bOpenCombo = true;  // reopen combo with new selected index at center
        } else if (item.DataFieldIndex == ComboList::ReopenLESSDataIndex) {
          // same as above but lower items needed
          ComboPopupDataField->SetDetachGUI(true);
          ComboListPopup->ItemIndex++;
          bOpenCombo = true;
        }
        ComboPopupDataField->SetFromCombo(item.DataFieldIndex, item.Value.c_str());
      }
      else {  // Cancel
              // if we've detached the GUI during the load, then there is nothing to do here

         // NOTE 130206 : we are missing currently the Cancel return status .
        // The list selection does not return the Cancel button status, because so far we have been setting an empty
        // value on entry, and we check on exit if it is still empty. In such case, a no/action is performed, either
        // because the user did not select anything with Select (click on empty field at the top) or because he really
        // clicked on Cancel and we returned again the initial empty value.
        // BUT, in some cases, like on TaskOverview, we set the initial item of the list to Default.task,
        // and in this case a Cancel will return correctly Default.tsk!
        // This is why we get the confirmation message for loading default task, instead of a quiet return.
        // Solution: either use a WindowControl variable, or a more simple global for ComboCancel.
        // This would be an hack, but quick and dirty solution with no disde effects.
        // Set ComboCancel true if we are here, otherwise false, and check that after
        // dfe = (DataFieldFileReader*) wp->GetDataField()
        // If ever we want to manage this Cancel button correctly, we should use one of these approaches.

        LKASSERT(iSavedInitialDataIndex >= 0);
        if (iSavedInitialDataIndex >= 0) {
          ComboPopupDataField->SetFromCombo(iSavedInitialDataIndex, sSavedInitialValue);
        }
      }
    }

    wComboPopupWndProperty->RefreshDisplay();
    ComboListPopup->clear();
    ComboListPopup = nullptr;

  } // loop reopen combo if <<More>> << LESS>> picked

  wComboPopupWndProperty = nullptr;
  bInComboPicker=false;
  return 1;

}
