/*
   LK8000 Tactical Flight Computer -  WWW.LK8000.IT
   Released under GNU/GPL License v.2
   See CREDITS.TXT file for authors and copyrights

   $Id: dlgWindSettings.cpp,v 1.1 2011/12/21 10:29:29 root Exp root $
*/


#include "externs.h"
#include "Dialogs.h"
#include "TraceThread.h"

#include "externs.h"
#include "WindowControls.h"
#include "dlgTools.h"
#include "InputEvents.h"
#include "Dialogs.h"
#include "resource.h"
#include "NavFunctions.h"
#include "Util/TruncateString.hpp"

static int SqCnt=0;
static int HoldOff = 0;
#define HOLDOFF_TIME  1 // x 0.5s
#define VOL 0
#define SQL 1
#define STX 0x02

static unsigned char VolMode  = 0;
static unsigned char lVolume  = 6;
static unsigned char lSquelch = 3;

BOOL ValidFrequency(double Freq)
{
BOOL Valid =FALSE;
int Frac = 	(int)(Freq*1000.0+0.05) - 100*((int) (Freq *10.0+0.05));

  if(Freq >= 118.0)
    if(Freq <= 137.0)
      switch(Frac)
      {
        case 0:
        case 25:
        case 50:
        case 75:
          Valid = TRUE;
        break;

        case 5:
        case 10:
        case 15:
        case 30:
        case 35:
        case 40:
        case 55:
        case 60:
        case 65:
        case 80:
        case 85:
        case 90:
          if(RadioPara.Enabled8_33)
            Valid = TRUE;
        break;

        default:
        break;
      }


  return Valid;
}


int SearchStation(double Freq)
{
int i;
TCHAR	szFreq[8] ;
_stprintf(szFreq,  _T("%7.3f"),Freq);
	double minDist =9999999;
	int minIdx=0;
  //  LKASSERT(numvalidwp<=NumberOfWayPoints);
	double fDist, fBear;
	for (i=0; i<(int)WayPointList.size(); i++)
	{
                      LKASSERT(ValidWayPointFast(i));
	 //   LKASSERT(numvalidwp<=NumberOfWayPoints);

	    if (WayPointList[i].Latitude!=RESWP_INVALIDNUMBER)
	    {

	      DistanceBearing(GPS_INFO.Latitude,
	                      GPS_INFO.Longitude,
	                      WayPointList[i].Latitude,
	                      WayPointList[i].Longitude,
	                      &fDist,
	                      &fBear);
	      if(fabs(Freq -   StrToDouble(WayPointList[i].Freq,NULL)) < 0.001)
	        if(fDist < minDist)
	        {
		  minDist = fDist;
		  minIdx =i;
	        }
	    }
	}

	return minIdx;
}



#define DEVICE_NAME_LEN 12

static void OnCancelClicked(WndButton* pWnd){
  if(pWnd) {
    WndForm * pForm = pWnd->GetParentWndForm();
    if(pForm) {
      pForm->SetModalResult(mrOK);
    }
  }
}


static void OnCloseClicked(WndButton* pWnd){
  if(pWnd) {
    WndForm * pForm = pWnd->GetParentWndForm();
    if(pForm) {
      pForm->SetModalResult(mrOK);
    }
  }
}

// update Station name with waypoint name.
static void UpdateStationName() {
    constexpr TCHAR unknown_name[] =_T("  ???  ");

    if( _tcslen(RadioPara.ActiveName) == 0) {
      LockTaskData();
      int Idx = SearchStation(RadioPara.ActiveFrequency);
      const TCHAR* new_name = ((Idx != 0) ? WayPointList[Idx].Name : unknown_name);
      devPutFreqActive(RadioPara.ActiveFrequency, new_name);
      UnlockTaskData();
    }

    if( _tcslen(RadioPara.PassiveName) == 0) {
      LockTaskData();
      int Idx = SearchStation(RadioPara.PassiveFrequency);
      const TCHAR* new_name = ((Idx != 0) ? WayPointList[Idx].Name : unknown_name);
      devPutFreqActive(RadioPara.PassiveFrequency, new_name);
      UnlockTaskData();
    }
}

static
void SetNameCaption(WndForm* pForm, const TCHAR* WndName, const TCHAR* Name, bool Tx, bool Rx) {
  WindowControl* pWnd = pForm->FindByName(WndName);
  if(pWnd) {
    TCHAR Name[DEVICE_NAME_LEN + 16];
    TCHAR ShortName[DEVICE_NAME_LEN];
    CopyTruncateString(ShortName, DEVICE_NAME_LEN, Name);

    if(Tx) {
      _stprintf(Name,_T(">%s<"),ShortName);
    } else if(Rx) {
      _stprintf(Name,_T("<%s>"),ShortName);
    } else {
      _stprintf(Name,_T("[%s]"),ShortName);
    }
    pWnd->SetCaption(Name);
  }
}

static
void SetFrequencyCaption(WndForm* pForm, const TCHAR* WndName, double Frequency) {
  WindowControl* pWnd = pForm->FindByName(WndName);
  if(pWnd) {
    TCHAR Name[64];
    _stprintf(Name,_T("%6.03f"), Frequency);
    pWnd->SetCaption(Name);
  }
}

void SetVolumeCaption(WndForm* pForm, const TCHAR* WndName) {
  WindowControl* pWnd = pForm->FindByName(WndName);
  if (pWnd) {
    TCHAR Name[64];
    if (VolMode == VOL)
      _stprintf(Name, _T("V [%i]"), RadioPara.Volume);
    else
      _stprintf(Name, _T("S [%i]"), RadioPara.Squelch);
    pWnd->SetCaption(Name);
  }
}

static
void SetDualModeCaption(WndForm* pForm, const TCHAR* WndName) {
  WindowControl* pWnd = pForm->FindByName(WndName);
  if(pWnd) {
    // TODO : use localized string
    pWnd->SetCaption(RadioPara.Dual ? _T("[Dual Off]") : _T("[Dual On]"));
  }
}

static 
void SetAutoCaption(WndForm* pForm, const TCHAR* WndName, bool Auto) {
  WindowControl* pWnd = pForm->FindByName(WndName);
  if(pWnd) {
    pWnd->SetCaption(Auto ? MsgToken(1324) : _T(""));   //  M1324 "B>"
  }
}

static int OnRemoteUpdate(WndForm* pForm) {
  if(RadioPara.Changed) {
    RadioPara.Changed =FALSE;

    UpdateStationName();

    SetNameCaption(pForm, TEXT("cmdActive"), RadioPara.ActiveName, RadioPara.TX, RadioPara.RX_active);
    SetNameCaption(pForm, TEXT("cmdPassive"), RadioPara.PassiveName, false, RadioPara.RX_standy);

    SetFrequencyCaption(pForm, TEXT("cmdActiveFreq"), RadioPara.ActiveFrequency);
    SetFrequencyCaption(pForm, TEXT("cmdPassiveFreq"), RadioPara.PassiveFrequency);

    if( lVolume !=  RadioPara.Volume)
          VolMode = VOL;
    lSquelch =  RadioPara.Squelch;
    lVolume =  RadioPara.Volume;

    SetVolumeCaption(pForm, TEXT("cmdVol"));
    SetDualModeCaption(pForm, TEXT("cmdDual"));

    return 1;
  }
  return 0;
}

static int OnUpdate(WndForm* pForm) {

  SetNameCaption(pForm, TEXT("cmdActive"), RadioPara.ActiveName, RadioPara.TX, RadioPara.RX_active);
  SetNameCaption(pForm, TEXT("cmdPassive"), RadioPara.PassiveName, false, RadioPara.RX_standy);

  SetFrequencyCaption(pForm, TEXT("cmdActiveFreq"), RadioPara.ActiveFrequency);
  SetFrequencyCaption(pForm, TEXT("cmdPassiveFreq"), RadioPara.PassiveFrequency);

  SetVolumeCaption(pForm, TEXT("cmdVol"));

  SetAutoCaption(pForm, TEXT("cmdAutoActive"), bAutoActive);
  SetAutoCaption(pForm, TEXT("cmdAutoPassive"), bAutoPassiv);
  
  return 0;
}


static void OnDualButton(WndButton* pWnd){
  RadioPara.Dual = !RadioPara.Dual;
  devPutRadioMode((int)RadioPara.Dual);
  SetDualModeCaption(pWnd->GetParentWndForm(), _T("cmdDual"));
}


static void OnActiveButton(WndButton* pWnd){
  if (HoldOff ==0) {
    int res = dlgWayPointSelect(0, 90.0, 1, 3);
    if(ValidNotResWayPoint(res)) {
      const WAYPOINT& wpt = WayPointList[res];
      devPutFreqActive(wpt.Freq, wpt.Name);
    }
    OnUpdate(pWnd->GetParentWndForm());
    HoldOff = HOLDOFF_TIME;
  }
}


static void OnPassiveButton(WndButton* pWnd){
  if (HoldOff ==0) {
    int res = dlgWayPointSelect(0, 90.0, 1,3);
    if(ValidNotResWayPoint(res)) {
      const WAYPOINT& wpt = WayPointList[res];
      devPutFreqStandby(wpt.Freq, wpt.Name);
    }
    OnUpdate(pWnd->GetParentWndForm());
    HoldOff = HOLDOFF_TIME;
  }
}



static void OnActiveFreq(WndButton* pWnd){
TCHAR	szFreq[20];
_stprintf(szFreq, _T("%7.3f"),RadioPara.ActiveFrequency);
 TCHAR	Name[NAME_SIZE+1] = _T("  ???   ");
    dlgNumEntryShowModal(szFreq,8);
    double Frequency = StrToDouble(szFreq,NULL);
    while(Frequency > 1000.0)
	   Frequency /=10;
    if(ValidFrequency(Frequency))
    {
      int iIdx = SearchStation(Frequency);
      if(iIdx != 0)
      {
    	 	_tcscpy(Name, WayPointList[iIdx].Name);
      }
      devPutFreqActive(Frequency,Name);
    }
    OnUpdate(pWnd->GetParentWndForm());
}

static void OnPassiveFreq(WndButton* pWnd){
TCHAR	szFreq[20] ;
_stprintf(szFreq,  _T("%7.3f"),RadioPara.PassiveFrequency);
TCHAR	Name[NAME_SIZE+1] = _T("  ???   ");
   dlgNumEntryShowModal(szFreq,8);

   double Frequency = StrToDouble(szFreq,NULL);
   while(Frequency > 1000)
	   Frequency /=10;

   if(ValidFrequency(Frequency))
   {
     int iIdx = SearchStation(Frequency);
     if(iIdx != 0)
     {
     	 _tcscpy(Name, WayPointList[iIdx].Name);
     }
     devPutFreqStandby(Frequency,Name);
   }
   OnUpdate(pWnd->GetParentWndForm());
}


static void OnRadioActiveAutoClicked(WndButton* pWnd){
  if(bAutoActive) {
    bAutoActive = false;
  } else {
    bAutoActive = true;
    if ( ValidWayPoint(BestAlternate)) {
      const WAYPOINT& wpt = WayPointList[BestAlternate];
      devPutFreqActive(wpt.Freq, wpt.Name);
    }
  }
  OnUpdate(pWnd->GetParentWndForm());
}


static void OnRadioStandbyAutoClicked(WndButton* pWnd)
{
  if(bAutoPassiv) {
    bAutoPassiv = false;
  } else {
    bAutoPassiv = true;
    if ( ValidWayPoint(BestAlternate)) {
      const WAYPOINT& wpt = WayPointList[BestAlternate];
      devPutFreqStandby(wpt.Freq, wpt.Name);
    }
  }
  OnUpdate(pWnd->GetParentWndForm());
}


static void OnExchange(WndButton* pWnd){
  devPutFreqSwap();
  OnUpdate(pWnd->GetParentWndForm());
}

static void SendVolSq(void){
  if (HoldOff ==0)
  {

  }
}


static void OnMuteButton(WndButton* pWnd){

    switch (VolMode)
    {
      default:
      case VOL: VolMode = SQL;  SqCnt =0; break;
      case SQL: VolMode = VOL;  SqCnt =11;break;
    }
    OnUpdate(pWnd->GetParentWndForm());
}


static void OnVolUpButton(WndButton* pWnd){

    if(VolMode == VOL)
    {
        if(lVolume < 20)
          lVolume += 1;
        if(lVolume > 20) lVolume = 20;
        if (HoldOff ==0)
        {
          devPutVolume(lVolume);
          HoldOff = HOLDOFF_TIME;
        }
    }
    else
    {
      if(lSquelch < 10)
        lSquelch += 1;
      if(lSquelch > 10) lSquelch = 10;
      SqCnt =0;
      if (HoldOff ==0)
      {
        devPutSquelch(lSquelch);
        HoldOff = HOLDOFF_TIME;
      }
    }
    OnUpdate(pWnd->GetParentWndForm());
}



static void OnVolDownButton(WndButton* pWnd){

  if(VolMode == VOL)
  {
	if(lVolume > 1)
	  lVolume -= 1;
	if(lVolume < 1)
	  lVolume = 1;
	if (HoldOff ==0)
	{
	  devPutVolume(lVolume);
	  HoldOff = HOLDOFF_TIME;
	}
  }
  else
  {
	if(lSquelch > 1)
		lSquelch -= 1;
	if(lSquelch < 1) lSquelch = 1;
	SqCnt =0;
	  if (HoldOff ==0)
	  {
	      devPutSquelch(lSquelch);
	      HoldOff = HOLDOFF_TIME;
	  }
  }

  OnUpdate(pWnd->GetParentWndForm());
  SendVolSq();
  HoldOff = HOLDOFF_TIME;
}



static bool OnTimerNotify(WndForm* pWnd) {

  if(VolMode != VOL)
    SqCnt++;

  if(SqCnt > 10)
  {
    VolMode = VOL;
    SqCnt = 0;
    OnUpdate(pWnd);
  }
  if (HoldOff >0)
    HoldOff--;
  else
    OnRemoteUpdate(pWnd);


  return 0;
}


static CallBackTableEntry_t CallBackTable[]={
  ClickNotifyCallbackEntry(OnDualButton),
  ClickNotifyCallbackEntry(OnActiveButton),
  ClickNotifyCallbackEntry(OnActiveFreq),
  ClickNotifyCallbackEntry(OnPassiveFreq),
  ClickNotifyCallbackEntry(OnPassiveButton),
  ClickNotifyCallbackEntry(OnMuteButton),
  ClickNotifyCallbackEntry(OnCancelClicked),
  ClickNotifyCallbackEntry(OnCloseClicked),
  ClickNotifyCallbackEntry(OnRadioActiveAutoClicked),
  ClickNotifyCallbackEntry(OnRadioStandbyAutoClicked),
  ClickNotifyCallbackEntry(OnActiveButton),
  ClickNotifyCallbackEntry(OnActiveFreq),
  ClickNotifyCallbackEntry(OnPassiveButton),
  ClickNotifyCallbackEntry(OnPassiveFreq),
  ClickNotifyCallbackEntry(OnMuteButton),
  ClickNotifyCallbackEntry(OnDualButton),
  ClickNotifyCallbackEntry(OnVolDownButton),
  ClickNotifyCallbackEntry(OnVolUpButton),
  ClickNotifyCallbackEntry(OnExchange),
  EndCallBackEntry()
};


void dlgRadioSettingsShowModal(void){
  SHOWTHREAD(_T("dlgRadioSettingsShowModal"));

  std::unique_ptr<WndForm> pForm(dlgLoadFromXML(CallBackTable, IDR_XML_RADIOSETTINGS ));
  if (!pForm) return;

  VolMode = VOL; // start with volume



  pForm->SetTimerNotify(300, OnTimerNotify);
  OnUpdate(pForm.get());

  pForm->ShowModal();

}
