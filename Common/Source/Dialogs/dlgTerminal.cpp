/*
   LK8000 Tactical Flight Computer -  WWW.LK8000.IT
   Released under GNU/GPL License v.2 or later
   See CREDITS.TXT file for authors and copyrights

   $Id$
*/

#include "externs.h"
#include "Dialogs.h"
#include "WindowControls.h"
#include <ctype.h>
#include "dlgTools.h"
#include "ComCheck.h"
#include "resource.h"


static WndForm *wf=NULL;
static WndListFrame *wTTYList=NULL;
static WndOwnerDrawFrame *wTTYListEntry = NULL;
static TCHAR TxText[MAX_NMEA_LEN] =_T("");
static TCHAR tmps[100];
TCHAR* DeviceName(int dev)
{
  static TCHAR NewName[50];
  lk::strcpy(NewName,MsgToken<232>());
  NewName[(_tcslen(NewName))-1] ='A'+dev;
  return NewName;
}
#define INTERLINE NIBLSCALE(2);

static bool OnTimerNotify(WndForm* pWnd) {
    wTTYList->Redraw();
    return true;
}


static void OnPaintListItem(WndOwnerDrawFrame * Sender, LKSurface& Surface){

  unsigned int hline, hframe, numlines;
  short active;

  Surface.SetTextColor(RGB_BLACK);
  Surface.SelectObject(LK8GenericVar02Font);
  active=ComCheck_ActivePort; // can change in thread
  if (active<0) return;

  hline = Surface.GetTextHeight(TEXT("M"))+INTERLINE;
  hframe=wTTYListEntry->GetHeight();
  numlines=hframe/hline -1;
  if (numlines>=CC_NUMBUFLINES) numlines=CC_NUMBUFLINES-1;

  unsigned int y=0, first, last;

  _stprintf(tmps,_T("[ Rx=%u Tx=%u ErrRx=%u ErrTx=%u ]"),
      DeviceList[active].Rx ,  DeviceList[active].Tx,  DeviceList[active].ErrRx ,  DeviceList[active].ErrTx);
  Surface.DrawText(0, 0, tmps);
  y+=hline;


  if (ComCheck_Reset>=0 || (ComCheck_LastLine==0 && ComCheck_BufferFull==false)) {
      _stprintf(tmps,_T("%s"),MsgToken<1872>()); // NO DATA RECEIVED
      Surface.DrawText(0, y, tmps);
      return;
  }

  last=ComCheck_LastLine;
  if (ComCheck_BufferFull==false && ComCheck_LastLine<=numlines) {
      first=0;
  } else {
      int dif = ComCheck_LastLine-numlines;
      if (dif>=0)
          first=dif;
      else
          first= (CC_NUMBUFLINES-1) + dif;
  }

  for (unsigned int n=0, curr=first; n<numlines; n++) {
      BUGSTOP_LKASSERT(curr<CC_NUMBUFLINES);
      if (curr>=CC_NUMBUFLINES) break;
      Surface.DrawTextClip(0, y, ComCheckBuffer[curr], wTTYListEntry->GetWidth());
      y+=hline;
      if (curr==last) break;
      if (++curr>(CC_NUMBUFLINES-1)) curr=0;
  }

}


static bool stopped=false;


static void OnPortClicked(WndButton* pWnd) {
  // Name is available only in Fly mode, not inited in SIM mode because no devices, and not inited if disabled
  _stprintf(tmps,_T("%s: %s (%s)"),MsgToken<1871>(),DeviceName(SelectedDevice),
     _tcslen(DeviceList[SelectedDevice].Name)>0?DeviceList[SelectedDevice].Name:MsgToken<1600>());
  wf->SetCaption(tmps);
  ComCheck_ActivePort=SelectedDevice; // needed
  ComCheck_Reset=ComCheck_ActivePort;
  wf->SetTimerNotify(500, OnTimerNotify);
  (wf->FindByName<WndButton>(TEXT("cmdSelectStop")))->SetCaption(MsgToken<670>()); // Stop
  stopped=false;
}



static void OnStopClicked(WndButton* pWnd) {
  stopped=!stopped;
  wf->SetCaption(tmps);
  if (stopped) {
      _stprintf(tmps,_T("%s: %s  %s"),MsgToken<1871>(), MsgToken<670>(),
          DeviceName(SelectedDevice));
      wf->SetCaption(tmps);
      wf->SetTimerNotify(0, NULL);
      (wf->FindByName<WndButton>(TEXT("cmdSelectStop")))->SetCaption(MsgToken<1200>()); // Start
  } else {
      {
        _stprintf(tmps,_T("%s: %s (%s)"),MsgToken<1871>(),DeviceName(SelectedDevice),
        _tcslen(DeviceList[SelectedDevice].Name)>0?DeviceList[SelectedDevice].Name:MsgToken<1600>());

      }
      wf->SetCaption(tmps);
      wf->SetTimerNotify(500, OnTimerNotify);
      (wf->FindByName<WndButton>(TEXT("cmdSelectStop")))->SetCaption(MsgToken<670>()); // Stop
  }
}


static void OnPrevClicked(WndButton* pWnd) {
    if(!stopped)
      OnStopClicked(pWnd);

    if(SelectedDevice==0)
      SelectedDevice = NUMDEV-1;
    else
      SelectedDevice--;
    OnPortClicked(pWnd);
 //   OnStopClicked(pWnd);

}

static void OnNextClicked(WndButton* pWnd) {
  if(!stopped) {
    OnStopClicked(pWnd);
  }
  SelectedDevice++;
  if(SelectedDevice==NUMDEV) {
    SelectedDevice = 0;
  }

  OnPortClicked(pWnd);
  //  OnStopClicked(pWnd);
}


static void OnTTYCloseClicked(WndButton* pWnd) {
  ComCheck_ActivePort=-1;
  if(pWnd) {
    WndForm * pForm = pWnd->GetParentWndForm();
    if(pForm) {
      pForm->SetModalResult(mrCancel);
    }
  }
}


static CallBackTableEntry_t CallBackTable[]={
  CallbackEntry(OnPaintListItem),
  EndCallbackEntry()
};





int AddCheckSumStrg( TCHAR szStrg[] )
{
int i,iCheckSum=0;
TCHAR  szCheck[254];

 if(szStrg[0] != '$')
 {
   _tcscat(szStrg,_T("\r\n"));
   return -1;
 }

  if ( _tcscmp(szStrg,_T("$$$")) == 0)  { // sent "$$$" probably for puting a RN42 BT chip in command mode
    _tcscat(szStrg,_T("\r\n"));
    return -1;
  }

  iCheckSum = szStrg[1];
  for (i=2; i < (int)_tcslen(szStrg); i++)
  {
	 if(szStrg[i] == '*')
	 {
		 szStrg[i] = 0;
		 continue;
	 }
	 else
	  iCheckSum ^= szStrg[i];
  }
  _stprintf(szCheck,TEXT("*%02X\r\n"),iCheckSum);
  _tcscat(szStrg,szCheck);
  return iCheckSum;
}


static void OnSendClicked(WndButton* pWnd) {

WndForm* pOwner = pWnd->GetParentWndForm();
WndProperty* wp = NULL;
  if(pOwner)
	wp = pOwner->FindByName<WndProperty>(TEXT("prpSendText"));

  if(wp)
  {
    TCHAR Tmp[MAX_NMEA_LEN] =_T(""); 
    short active=ComCheck_ActivePort; // can change in thread
    _tcsncpy(Tmp, wp->GetDataField()->GetAsString(), MAX_NMEA_LEN - 6);
    Tmp[MAX_NMEA_LEN - 6]= '\0'; // additional space for the checksum
    lk::strcpy(TxText, Tmp);

    AddCheckSumStrg(Tmp);

    if((ComCheck_ActivePort >= 0) && (DeviceList[active].Com))
    {
      DeviceList[active].Com->WriteString(Tmp);
      ComCheck_AddText(_T("\r\n > "));
      ComCheck_AddText(Tmp);
    }
  }
}




void dlgTerminal(int portnumber) {

  wf = dlgLoadFromXML(CallBackTable, ScreenLandscape ? IDR_XML_TERMINAL_L : IDR_XML_TERMINAL_P);

  if (!wf) return;


  (wf->FindByName<WndButton>(TEXT("cmdClose")))->SetOnClickNotify(OnTTYCloseClicked);
  (wf->FindByName<WndButton>(TEXT("cmdSelectPort1")))->SetOnClickNotify(OnPrevClicked);
  (wf->FindByName<WndButton>(TEXT("cmdSelectPort2")))->SetOnClickNotify(OnNextClicked);
  (wf->FindByName<WndButton>(TEXT("cmdSelectStop")))->SetOnClickNotify(OnStopClicked);
  (wf->FindByName<WndButton>(TEXT("cmdSendButton")))->SetOnClickNotify(OnSendClicked);

  wTTYList = wf->FindByName<WndListFrame>(TEXT("frmTTYList"));
  LKASSERT(wTTYList!=NULL);
  wTTYList->SetWidth(wf->GetWidth()-NIBLSCALE(2));

  wTTYListEntry = wf->FindByName<WndOwnerDrawFrame>(TEXT("frmTTYListEntry"));
  LKASSERT(wTTYListEntry!=NULL);
  wTTYListEntry->SetWidth(wf->GetWidth()-NIBLSCALE(2));

  OnPortClicked(NULL);


  if(wf)
  {
	WndProperty* wp = wf->FindByName<WndProperty>(TEXT("prpSendText"));
    wp->GetDataField()->Set(TxText);
    wp->RefreshDisplay();
  }

  wf->ShowModal();

  ComCheck_ActivePort=-1;
  delete wf;
  wf = NULL;
  return;

}
