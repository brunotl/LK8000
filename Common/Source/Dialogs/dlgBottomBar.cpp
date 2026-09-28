/*
   LK8000 Tactical Flight Computer -  WWW.LK8000.IT
   Released under GNU/GPL License v.2 or later
   See CREDITS.TXT file for authors and copyrights

   $Id: dlgBottomBar.cpp,v 1.1 2011/12/21 10:29:29 root Exp root $
*/

#include "externs.h"
#include "LKProfiles.h"
#include "Dialogs.h"
#include "dlgTools.h"
#include "WindowControls.h"
#include "Terrain.h"
#include "LKMapWindow.h"
#include "LKInterface.h"
#include "resource.h"

static bool changed = false;

static void OnCloseClicked(WndButton* pWnd) {
  if(pWnd) {
    WndForm * pForm = pWnd->GetParentWndForm();
    if(pForm) {
      pForm->SetModalResult(mrOK);
    }
  }
}


static void setVariables(WndForm* wf) {
  WndProperty *wp;

  wp = wf->FindByName<WndProperty>(TEXT("prpConfBB0"));
  if (wp) {
    DataField* dfb = wp->GetDataField();
    dfb->Set(ConfBB[0]);
    wp->RefreshDisplay();
  }
  wp = wf->FindByName<WndProperty>(TEXT("prpConfBB1"));
  if (wp) {
    DataField* dfb = wp->GetDataField();
    dfb->Set(ConfBB[1]);
    wp->RefreshDisplay();
  }
  wp = wf->FindByName<WndProperty>(TEXT("prpConfBB2"));
  if (wp) {
    DataField* dfb = wp->GetDataField();
    dfb->Set(ConfBB[2]);
    wp->RefreshDisplay();
  }
  wp = wf->FindByName<WndProperty>(TEXT("prpConfBB3"));
  if (wp) {
    DataField* dfb = wp->GetDataField();
    dfb->Set(ConfBB[3]);
    wp->RefreshDisplay();
  }
  wp = wf->FindByName<WndProperty>(TEXT("prpConfBB4"));
  if (wp) {
    DataField* dfb = wp->GetDataField();
    dfb->Set(ConfBB[4]);
    wp->RefreshDisplay();
  }
  wp = wf->FindByName<WndProperty>(TEXT("prpConfBB5"));
  if (wp) {
    DataField* dfb = wp->GetDataField();
    dfb->Set(ConfBB[5]);
    wp->RefreshDisplay();
  }
  wp = wf->FindByName<WndProperty>(TEXT("prpConfBB6"));
  if (wp) {
    DataField* dfb = wp->GetDataField();
    dfb->Set(ConfBB[6]);
    wp->RefreshDisplay();
  }
  wp = wf->FindByName<WndProperty>(TEXT("prpConfBB7"));
  if (wp) {
    DataField* dfb = wp->GetDataField();
    dfb->Set(ConfBB[7]);
    wp->RefreshDisplay();
  }
  wp = wf->FindByName<WndProperty>(TEXT("prpConfBB8"));
  if (wp) {
    DataField* dfb = wp->GetDataField();
    dfb->Set(ConfBB[8]);
    wp->RefreshDisplay();
  }
  wp = wf->FindByName<WndProperty>(TEXT("prpConfBB9"));
  if (wp) {
    DataField* dfb = wp->GetDataField();
    dfb->Set(ConfBB[9]);
    wp->RefreshDisplay();
  }
  wp = wf->FindByName<WndProperty>(TEXT("prpConfBB0Auto"));
  if (wp) {
    DataField* dfe = wp->GetDataField();
    dfe->addEnumText(MsgToken<2252>());  // MANUAL
    dfe->addEnumText(MsgToken<2253>()); //  AUTO THERMALLING
    dfe->addEnumText(MsgToken<2254>()); //  FULL AUTO
    dfe->Set(ConfBB0Auto);
    wp->RefreshDisplay();
  }

}


static CallBackTableEntry_t CallBackTable[]={
  CallbackEntry(OnCloseClicked),
  EndCallbackEntry()
};


void dlgBottomBarShowModal(void){

  WndProperty *wp;

  WndForm* wf = dlgLoadFromXML(CallBackTable, IDR_XML_BOTTOMBAR);

  if (!wf) return;

  setVariables(wf);

  changed = false;

  wf->ShowModal();

  wp = wf->FindByName<WndProperty>(TEXT("prpConfBB0"));
  if (wp) {
	if (ConfBB[0] != (wp->GetDataField()->GetAsBoolean())) {
		ConfBB[0] = (wp->GetDataField()->GetAsBoolean());
		changed=true;
	}
  }
  wp = wf->FindByName<WndProperty>(TEXT("prpConfBB1"));
  if (wp) {
	if (ConfBB[1] != (wp->GetDataField()->GetAsBoolean())) {
		ConfBB[1] = (wp->GetDataField()->GetAsBoolean());
		changed=true;
	}
  }

  wp = wf->FindByName<WndProperty>(TEXT("prpConfBB2"));
  if (wp) {
	if (ConfBB[2] != (wp->GetDataField()->GetAsBoolean())) {
		ConfBB[2] = (wp->GetDataField()->GetAsBoolean());
		changed=true;
	}
  }

  wp = wf->FindByName<WndProperty>(TEXT("prpConfBB3"));
  if (wp) {
	if (ConfBB[3] != (wp->GetDataField()->GetAsBoolean())) {
		ConfBB[3] = (wp->GetDataField()->GetAsBoolean());
		changed=true;
	}
  }

  wp = wf->FindByName<WndProperty>(TEXT("prpConfBB4"));
  if (wp) {
	if (ConfBB[4] != (wp->GetDataField()->GetAsBoolean())) {
		ConfBB[4] = (wp->GetDataField()->GetAsBoolean());
		changed=true;
	}
  }

  wp = wf->FindByName<WndProperty>(TEXT("prpConfBB5"));
  if (wp) {
	if (ConfBB[5] != (wp->GetDataField()->GetAsBoolean())) {
		ConfBB[5] = (wp->GetDataField()->GetAsBoolean());
		changed=true;
	}
  }

  wp = wf->FindByName<WndProperty>(TEXT("prpConfBB6"));
  if (wp) {
	if (ConfBB[6] != (wp->GetDataField()->GetAsBoolean())) {
		ConfBB[6]  = (wp->GetDataField()->GetAsBoolean());
		changed=true;
	}
  }

  wp = wf->FindByName<WndProperty>(TEXT("prpConfBB7"));
  if (wp) {
	if (ConfBB[7] != (wp->GetDataField()->GetAsBoolean())) {
		ConfBB[7] = (wp->GetDataField()->GetAsBoolean());
		changed=true;
	}
  }

  wp = wf->FindByName<WndProperty>(TEXT("prpConfBB8"));
  if (wp) {
	if (ConfBB[8] != (wp->GetDataField()->GetAsBoolean())) {
		ConfBB[8] = (wp->GetDataField()->GetAsBoolean());
		changed=true;
	}
  }

  wp = wf->FindByName<WndProperty>(TEXT("prpConfBB9"));
  if (wp) {
	if (ConfBB[9] != (wp->GetDataField()->GetAsBoolean())) {
		ConfBB[9] = (wp->GetDataField()->GetAsBoolean());
		changed=true;
	}
  }

  wp = wf->FindByName<WndProperty>(TEXT("prpConfBB0Auto"));
  if (wp) {
    if (ConfBB0Auto != wp->GetDataField()->GetAsInteger() )
    {
      ConfBB0Auto = wp->GetDataField()->GetAsInteger();
    }
  }

  if (changed) {
    bool all_off = std::none_of(&ConfBB[1], &ConfBB[10], [](auto v) {
      return v;
    });
    if (all_off) {
      MessageBoxX(MsgToken<16>(),   // can't disable all non-TRM0
                  _T(""), mbOk);  // bottom bar stripes
      // Automatically enable NAV1 bottom bar
      ConfBB[1] = true;
    }

    if (UpdateConfBB()) {
      MessageBoxX(MsgToken<16>(),   // can't disable all non-TRM0
                  _T(""), mbOk);  // bottom bar stripes
    }

    MessageBoxX(MsgToken<1607>(),  // bottom bar config saved
                _T(""), mbOk);
  }

  delete wf;
}
