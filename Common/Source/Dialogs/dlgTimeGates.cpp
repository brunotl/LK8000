/*
   LK8000 Tactical Flight Computer -  WWW.LK8000.IT
   Released under GNU/GPL License v.2 or later
   See CREDITS.TXT file for authors and copyrights

   $Id: dlgTimeGates.cpp,v 1.1 2011/12/21 10:29:29 root Exp root $
*/

#include "externs.h"
#include "LKProcess.h"
#include "LKProfiles.h"
#include "Calculations2.h"
#include "Dialogs.h"
#include "WindowControls.h"
#include "dlgTools.h"
#include "resource.h"
#include "Calc/Task/TimeGates.h"

namespace {

void OnCloseClicked(WndButton* pWnd) {
  if(pWnd) {
    WndForm * pForm = pWnd->GetParentWndForm();
    if(pForm) {
      pForm->SetModalResult(mrOK);
    }
  }
}

void UpdateGateTypeUI(WndForm* pForm) {
  if (pForm) {
    auto frmFixed = pForm->FindByName(_T("frmFixed"));
    if (frmFixed) {
      frmFixed->SetVisible(TimeGates::GateType == TimeGates::fixed_gates);
    }
    auto frmPev = pForm->FindByName(_T("frmPev"));
    if (frmPev) {
      frmPev->SetVisible(TimeGates::GateType == TimeGates::pev_start);
    }
  }
}

void OnGateType(DataField* Sender, DataField::DataAccessKind_t Mode) {

  if(Sender->getCount() == 0) {
    Sender->addEnumList({
        TEXT("Anytime"),
        TEXT("Fixed Gates"),
        TEXT("PEV Start")
      });
  }

  auto& wp = Sender->GetOwner();
  auto pForm = wp.GetParentWndForm();

  switch (Mode) {
    case DataField::daGet:
      Sender->Set(TimeGates::GateType);
      break;
    case DataField::daChange:
      TimeGates::GateType = static_cast<TimeGates::open_type>(Sender->GetAsInteger());
      UpdateGateTypeUI(pForm);
      break;
    default:
      break;
  }
}

void OnWaitingTime(DataField* Sender, DataField::DataAccessKind_t Mode) {
  switch (Mode) {
    case DataField::daGet:
      Sender->Set(TimeGates::WaitingTime);
      break;
    case DataField::daChange:
      TimeGates::WaitingTime = Sender->GetAsInteger();
      break;
    default:
      break;
  }
}

void OnStartWindow(DataField* Sender, DataField::DataAccessKind_t Mode) {
  switch (Mode) {
    case DataField::daGet:
      Sender->Set(TimeGates::StartWindow);
      break;
    case DataField::daChange:
      TimeGates::StartWindow = Sender->GetAsInteger();
      break;
    default:
      break;
  }
}


CallBackTableEntry_t CallBackTable[]={
  CallbackEntry(OnCloseClicked),
  CallbackEntry(OnGateType),
  CallbackEntry(OnWaitingTime),
  CallbackEntry(OnStartWindow),
  EndCallbackEntry()
};

void setVariables(WndForm* pForm) {
  auto wp = pForm->FindByName<WndProperty>(TEXT("prpPGOptimizeRoute"));
  if (wp) {
    DataField* dfe = wp->GetDataField();
    if (dfe) {
      dfe->SetAsBoolean(TskOptimizeRoute);
    }
    wp->SetVisible(gTaskType == task_type_t::GP);
    wp->RefreshDisplay();
  }

  wp = pForm->FindByName<WndProperty>(TEXT("prpPGNumberOfGates"));
  if (wp) {
    wp->GetDataField()->SetAsInteger(TimeGates::PGNumberOfGates);
    wp->RefreshDisplay();
  }
  wp = pForm->FindByName<WndProperty>(TEXT("prpPGOpenTimeH"));
  if (wp) {
    wp->GetDataField()->SetAsInteger(TimeGates::PGOpenTimeH);
    wp->RefreshDisplay();
  }
  wp = pForm->FindByName<WndProperty>(TEXT("prpPGOpenTimeM"));
  if (wp) {
    wp->GetDataField()->SetAsInteger(TimeGates::PGOpenTimeM);
    wp->RefreshDisplay();
  }
  wp = pForm->FindByName<WndProperty>(TEXT("prpPGCloseTimeH"));
  if (wp) {
    wp->GetDataField()->SetAsInteger(TimeGates::PGCloseTimeH);
    wp->RefreshDisplay();
  }
  wp = pForm->FindByName<WndProperty>(TEXT("prpPGCloseTimeM"));
  if (wp) {
    wp->GetDataField()->SetAsInteger(TimeGates::PGCloseTimeM);
    wp->RefreshDisplay();
  }
  wp = pForm->FindByName<WndProperty>(TEXT("prpPGGateIntervalTime"));
  if (wp) {
    wp->GetDataField()->SetAsInteger(TimeGates::PGGateIntervalTime);
    wp->RefreshDisplay();
  }
}

} // namespace

void dlgTimeGatesShowModal() {
  std::unique_ptr<WndForm> pForm(dlgLoadFromXML(CallBackTable, IDR_XML_TIMEGATES));
  if (!pForm) {
    return;
  }
 
  setVariables(pForm.get());
  UpdateGateTypeUI(pForm.get());

  bool changed = false;

  pForm->ShowModal();
  // TODO enhancement: implement a cancel button that skips all this below after exit.

  auto wp = pForm->FindByName<WndProperty>(TEXT("prpPGOptimizeRoute"));
  if (wp) {
    if (TskOptimizeRoute != (wp->GetDataField()->GetAsBoolean())) {
      TskOptimizeRoute = (wp->GetDataField()->GetAsBoolean());
      changed = true;

      if (gTaskType == task_type_t::GP) {
        ClearOptimizedTargetPos();
      }
    }
  }

  auto frmFixed = pForm->FindByName(_T("frmFixed"));
  if (frmFixed) {
    frmFixed->SetVisible(TimeGates::GateType == TimeGates::fixed_gates);
  }
  auto frmPev = pForm->FindByName(_T("frmPev"));
  if (frmPev) {
    frmPev->SetVisible(TimeGates::GateType == TimeGates::pev_start);
  }
  wp = pForm->FindByName<WndProperty>(TEXT("prpPGNumberOfGates"));
  if (wp) {
    if (TimeGates::PGNumberOfGates != wp->GetDataField()->GetAsInteger()) {
      TimeGates::PGNumberOfGates = wp->GetDataField()->GetAsInteger();
      changed = true;
    }
  }
  wp = pForm->FindByName<WndProperty>(TEXT("prpPGOpenTimeH"));
  if (wp) {
    if (TimeGates::PGOpenTimeH != wp->GetDataField()->GetAsInteger()) {
      TimeGates::PGOpenTimeH = wp->GetDataField()->GetAsInteger();
      changed = true;
    }
  }
  wp = pForm->FindByName<WndProperty>(TEXT("prpPGOpenTimeM"));
  if (wp) {
    if (TimeGates::PGOpenTimeM != wp->GetDataField()->GetAsInteger()) {
      TimeGates::PGOpenTimeM = wp->GetDataField()->GetAsInteger();
      changed = true;
    }
  }
  wp = pForm->FindByName<WndProperty>(TEXT("prpPGCloseTimeH"));
  if (wp) {
    if (TimeGates::PGCloseTimeH != wp->GetDataField()->GetAsInteger()) {
      TimeGates::PGCloseTimeH = wp->GetDataField()->GetAsInteger();
      changed = true;
    }
  }
  wp = pForm->FindByName<WndProperty>(TEXT("prpPGCloseTimeM"));
  if (wp) {
    if (TimeGates::PGCloseTimeM != wp->GetDataField()->GetAsInteger()) {
      TimeGates::PGCloseTimeM = wp->GetDataField()->GetAsInteger();
      changed = true;
    }
  }
  wp = pForm->FindByName<WndProperty>(TEXT("prpPGGateIntervalTime"));
  if (wp) {
    if (TimeGates::PGGateIntervalTime != wp->GetDataField()->GetAsInteger()) {
      TimeGates::PGGateIntervalTime = wp->GetDataField()->GetAsInteger();
      changed = true;
    }
  }

  if (changed) {
    InitActiveGate();
  }
}
