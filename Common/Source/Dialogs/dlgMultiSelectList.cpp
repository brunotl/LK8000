/*
   LK8000 Tactical Flight Computer -  WWW.LK8000.IT
   Released under GNU/GPL License v.2 or later
   See CREDITS.TXT file for authors and copyrights

   $Id: dlgSelectAirspaceList.cpp,v 1.1 2011/12/21 10:29:29 root Exp root $
 */

#include "externs.h"
#include "Dialogs.h"
#include "dlgTools.h"
#include "WindowControls.h"
#include "Multimap.h"
#include "resource.h"
#include "NavFunctions.h"
#include "Draw/ScreenProjection.h"
#include "InputEvents.h"
#include "Sound/Sound.h"
#include "LKStyle.h"

#include "Dialogs.h"
#include "Asset.hpp"
#include "Util/TruncateString.hpp"
#include "Library/Utm.h"
#include "utils/printf.h"

#define MAX_LEN 200
#define MAX_COMMENT 80
#define MAX_LIST_ITEMS 25

ListElement* pResult = NULL;

ListElement Elements[MAX_LIST_ITEMS];

int iNO_ELEMENTS = 0;
static WndForm *wf = NULL;
static WndListFrame *wMultiSelectListList = NULL;
static WndOwnerDrawFrame *wMultiSelectListListEntry = NULL;

static int ItemIndex = -1;

static int NoAirfields = 0;
static int NoOutlands = 0;
static int NoWaypoints = 0;
static int NoTaskPoints = 0;
static int NoAirspace = 0;
#ifdef FLARM_MS
#include "FlarmIdFile.h"
static int NoFlarm = 0;
#endif
#ifdef WEATHERST_MS
static int NoWeatherSt = 0;
#endif

#define MAX_AIRFILEDS 3
#define MAX_OUTLAND   3
#define MAX_WAYPOINTS 10
#define MAX_TASK      3
#define MAX_AIRSPACES 10
#define MAX_FLARMS    5
#define MAX_WEATHERST 5

static void UpdateList(void) {
    wMultiSelectListList->ResetList();
    wMultiSelectListList->Redraw();
}

static int DrawListIndex = 0;


// Keep the dialog list updated every second

static bool OnTimer(WndForm* pWnd) {
    wMultiSelectListList->Redraw();
    return true;
}

static void OnUpClicked(WndButton* Sender) {
    if (ItemIndex > 0) {
        ItemIndex--;
    } else {
        LKASSERT(iNO_ELEMENTS>0);
        ItemIndex = (iNO_ELEMENTS - 1);
    }
    LKSound(TEXT("LK_TICK.WAV"));
    wMultiSelectListList->SetItemIndexPos(ItemIndex);
    wMultiSelectListList->Redraw();
    wMultiSelectListListEntry->SetFocus();
}

static void OnDownClicked(WndButton* pWnd) {
    (void)pWnd;

    if (ItemIndex < (iNO_ELEMENTS - 1)) {
        ItemIndex++;
    } else {
        ItemIndex = 0;
    }
    LKSound(TEXT("LK_TOCK.WAV"));
    wMultiSelectListList->SetItemIndexPos(ItemIndex);
    wMultiSelectListList->Redraw();
    wMultiSelectListListEntry->SetFocus();
}

void dlgAddMultiSelectListDetailsDialog(int Index) {

    if ((Index >= 0) && (Index < iNO_ELEMENTS)) {
        const ListElement& el = Elements[Index];

        switch (el.type) {

#ifdef TEAM_CODE_MS
        case IM_TEAM:
            wf->SetTimerNotify(0,NULL);
            InputEvents::processPopupDetails(InputEvents::PopupTeam,el.iIdx);
            break;
#endif
#ifdef ORACLE_MS
        case IM_ORACLE:
            wf->SetTimerNotify(0,NULL);
            InputEvents::processPopupDetails(InputEvents::PopupOracle,el.iIdx);
            break;
#endif
#ifdef OWN_POS_MS
        case IM_OWN_POS:
            wf->SetTimerNotify(0,NULL);
            InputEvents::processPopupDetails(InputEvents::PopupBasic,el.iIdx);
            break;
#endif
#ifdef FLARM_MS
        case IM_FLARM:
            wf->SetTimerNotify(0,NULL);
            LKASSERT(el.iIdx < (int)WayPointList.size());
            InputEvents::processPopupDetails(InputEvents::PopupTraffic,el.iIdx);
            wf->SetTimerNotify(0,NULL);
            break;
#endif
#ifdef WEATHERST_MS
        case IM_WEATHERST:
            wf->SetTimerNotify(0,NULL);
            InputEvents::processPopupDetails(InputEvents::PopupWeatherSt,el.iIdx);
            break;
#endif
        case IM_AIRSPACE:
            wf->SetTimerNotify(0,NULL);
            LKASSERT(el.ptr);
            CAirspaceManager::Instance().PopupAirspaceDetail(static_cast<CAirspace*>(el.ptr));
            break;

        case IM_WAYPOINT:
            wf->SetTimerNotify(0,NULL);
            LKASSERT(el.iIdx < (int)WayPointList.size());
            SelectedWaypoint = el.iIdx;
            wf->SetTimerNotify(0,NULL);
            PopupWaypointDetails();
            break;

        case IM_TASK_PT:
            wf->SetTimerNotify(0,NULL);
            LKASSERT(el.iIdx <= MAXTASKPOINTS);
            int iLastTaskPoint = 0;
            while (ValidTaskPoint(iLastTaskPoint)) {
                iLastTaskPoint++;
            }
            iLastTaskPoint--;
            RealActiveWaypoint = -1;
            if (el.iIdx == 0)
                dlgTaskWaypointShowModal(el.iIdx, 0, false, true);
            else {
                if (el.iIdx == iLastTaskPoint)
                    dlgTaskWaypointShowModal(el.iIdx, 2, false, true);
                else {
                    if ((UseAATTarget()) && (CALCULATED_INFO.Flying) && (!IsMultiMapNoMain())) {
                        wf->SetModalResult(mrOK);
                        wf->SetVisible(false);
                        dlgTarget(el.iIdx);
                    } else {
                        dlgTaskWaypointShowModal(el.iIdx, 1, false, true);
                    }
                }
            }
            break;
        } // switch
    } // if Index..
}

int dlgGetNoElements(void) {
    return iNO_ELEMENTS;
}

void dlgAddMultiSelectListItem(long* pNew, int Idx, char type, double Distance) {

    if (type == IM_TASK_PT) {
        int iLastTaskPoint = 0;
        while (ValidTaskPoint(iLastTaskPoint))
            iLastTaskPoint++;

        if (iLastTaskPoint < 2)
            return;

        if (ValidTaskPoint(PanTaskEdit))
            return;

        if (Task[Idx].Index == RESWP_PANPOS)
            return;
    }

    if (type == IM_WAYPOINT && Idx == RESWP_PANPOS) {
      return;
    }

    for (int i = 0; i < std::min(iNO_ELEMENTS, MAX_LIST_ITEMS); i++) {
      const ListElement& el = Elements[i];

      if (pNew != NULL && el.ptr == pNew) {
        return;
      }
      if (type != IM_AIRSPACE && type == el.type && Idx == el.iIdx) {
        return;
      }
    } // for

    bool full = false;
    if (iNO_ELEMENTS < MAX_LIST_ITEMS - 1) {
        switch (type) {
        case IM_AIRSPACE:
            if (NoAirspace < MAX_AIRSPACES)
                NoAirspace++;
            else
                full = true;

            break;
#ifdef FLARM_MS
        case IM_FLARM:
            if (NoFlarm < MAX_FLARMS)
              NoFlarm++;
            else
                full = true;

            break;
#endif
#ifdef WEATHERST_MS
        case IM_WEATHERST:
            if (NoWeatherSt < MAX_WEATHERST)
              NoWeatherSt++;
            else
                full = true;

            break;
#endif
        case IM_TASK_PT:
            if (NoTaskPoints < MAX_TASK)
                NoTaskPoints++;
            else
                full = true;

            break;

        case IM_WAYPOINT:
            if (WayPointCalc[Idx].IsLandable) {
                if (WayPointCalc[Idx].IsAirport) {
                    if (NoAirfields < MAX_AIRFILEDS)
                        NoAirfields++;
                    else
                        full = true;
                } else {
                    if (NoOutlands < MAX_OUTLAND)
                        NoOutlands++;
                    else
                        full = true;
                }
            } else {
                if (NoWaypoints < MAX_WAYPOINTS)
                    NoWaypoints++;
                else
                    full = true;
            }

            break;
        } // switch

        if (full) {
          for (int i = 0; i < std::min(iNO_ELEMENTS, MAX_LIST_ITEMS); i++) {
            ListElement& el = Elements[i];
            if (el.type == type && el.Dist > Distance) {
              el = { type, pNew, Idx, Distance, 0};
              return;
            }
          }
        } else {
          // find element position
          int Pos = 0;
          while ((Pos < std::min(iNO_ELEMENTS, MAX_LIST_ITEMS)) && (Elements[Pos].Dist <= Distance)) {
            Pos++;
          }

          for (int i = std::min(iNO_ELEMENTS, MAX_LIST_ITEMS); i > Pos; i--) {
            Elements[i] = Elements[i - 1];
          }

          Elements[Pos] = { type, pNew, Idx, Distance, 0};
          iNO_ELEMENTS = std::min(iNO_ELEMENTS + 1, MAX_LIST_ITEMS);
        }

    } // if no elements..
}


#ifdef FLARM_MS
static
int BuildFLARMText(FLARM_TRAFFIC* pFlarm, TCHAR (&text1)[MAX_LEN],TCHAR (&text2)[MAX_LEN]) {
  if(pFlarm == NULL) return -1;

  TCHAR Comment[MAX_COMMENT] = _T("");

  double Distance, Bear;
  DistanceBearing( GPS_INFO.Latitude,GPS_INFO.Longitude, pFlarm->Latitude,  pFlarm->Longitude, &Distance, &Bear);
  if(_tcscmp(pFlarm->Name,_T("?")) ==0)
    lk::snprintf(text1, _T("%X"), pFlarm->RadioId);
  else
    lk::snprintf(text1, _T("[%s] %X"),pFlarm->Name, pFlarm->RadioId);

  const FlarmId* flarmId = LookupFlarmId(pFlarm->RadioId);
  if(flarmId != NULL)
  {
    if(flarmId->freq[3] != ' ')
      lk::snprintf(Comment, _T("%s  %s %s"), flarmId->type     // FLARMID_SIZE_TYPE   22
                                            , flarmId->freq     // FLARMID_SIZE_FREQ   8    r
                                            , flarmId->name);   // FLARMID_SIZE_NAME   22 => max 52 char
    else
      lk::snprintf(Comment, _T("%s %s"), flarmId->type            // FLARMID_SIZE_TYPE   22
                                     , flarmId->name);          // FLARMID_SIZE_NAME   22 => max 52 char

    lk::snprintf(text1, _T("%s [%s] %s "), 
                                    pFlarm->Cn,  // 4
                                    pFlarm->Name // 31
                                    , Comment); // 80 => Total 121
  }

  lk::snprintf(text2, TEXT("%3.1f%s  (%i%s  %3.1f%s  %i°) %s")
            , Units::ToDistance(Distance)                   //        6
            , Units::GetDistanceName()                          // 2+3=   5
            , (int)Units::ToAltitude(pFlarm->Altitude)      //        5
            , Units::GetAltitudeName()                          // 3+2=   5
            , Units::ToVerticalSpeed(pFlarm->Average30s)    //        5
            , Units::GetVerticalSpeedName()                     // 3+2=   5
            , (int) Bear
            , (flarmId ? flarmId->airfield : _T("")) );         //FLARMID_SIZE_AIRFIELD           22  =>  53 char
  return 0;
}
#endif

#ifdef WEATHERST_MS
static
int BuildWEATHERText(FANET_WEATHER* pWeather, TCHAR (&text1)[MAX_LEN],TCHAR (&text2)[MAX_LEN], const TCHAR* name)
{
  if(pWeather == NULL) return -1;

  TCHAR Comment[MAX_COMMENT] = _T("");;
  double Distance, Bear;
  DistanceBearing( GPS_INFO.Latitude,GPS_INFO.Longitude, pWeather->Latitude,  pWeather->Longitude, &Distance, &Bear);
  float press = pWeather->pressure;
  lk::snprintf(Comment, _T("%d° %d|%d %3.3f%s"), 
            (int)round(pWeather->windDir),
            (int)round(Units::ToWindSpeed(pWeather->windSpeed)),
            (int)round(Units::ToWindSpeed(pWeather->windGust)),
            Units::ToPressure(press), Units::GetPressureName());

  if(_tcslen(name) == 0) {
    lk::snprintf(text1, _T("%X %s"),pWeather->ID,Comment);
  }
  else {
    lk::snprintf(text1, _T("%s %s"),name,Comment);
  }

  lk::snprintf(Comment, _T("%d°C %d%% %d%%"), (int)round(pWeather->temp)
                                            , (int)round(pWeather->hum)
                                            , (int)round(pWeather->Battery));
  lk::snprintf(text2, _T("%3.1f%s (%i°) %s"),
            Units::ToDistance(Distance),
            Units::GetDistanceName(),
            (int) Bear , Comment);
  return 0;
}
#endif

static
int BuildTaskPointText(int iTaskIdx, TCHAR (&text1)[MAX_LEN], TCHAR (&text2)[MAX_LEN])
{
  int idx=0;
  if(ValidTaskPointFast(iTaskIdx)) {
      idx = Task[iTaskIdx].Index;
  } else return -1;

  int iLastTaskPoint = 0;

  while (ValidTaskPoint(iLastTaskPoint))
      iLastTaskPoint++;

  iLastTaskPoint--;

  if (iTaskIdx == 0) {
     // _@M2301_  "S"    # S = Start Task point
    lk::snprintf(text1, _T("%s: (%s)"), MsgToken<2301>(), WayPointList[idx].Name);
    lk::snprintf(text2, _T("Radius %3.1f%s (%i%s)"),
               Units::ToDistance(StartRadius), Units::GetDistanceName(),
               (int) Units::ToAltitude(WayPointList[idx].Altitude),
               Units::GetAltitudeName());
  } else {
     if (iTaskIdx == iLastTaskPoint) {
         //  _@M2303_  "F"                 // max 30         30 => max 60 char
         lk::snprintf(text1, _T("%s: (%s) "), MsgToken<2303>(),
                   WayPointList[idx].Name);
         lk::snprintf(text2, _T("Radius %3.1f%s (%i%s)"),
                   Units::ToDistance(FinishRadius), Units::GetDistanceName(),
                   (int) Units::ToAltitude(WayPointList[idx].Altitude),
                   Units::GetAltitudeName());
     } else {
         //   _@M2302_  "T"    # F = Finish point            // max 30         30 => max 60 char
         lk::snprintf(text1, _T("%s%i: (%s) "), MsgToken<2302>(), iTaskIdx,
                   WayPointList[idx].Name);
         double SecRadius = 0;

         SecRadius = SectorRadius;
         if (UseAATTarget()) {
             if (Task[iTaskIdx].AATType == sector_type_t::SECTOR)
                 SecRadius = Task[iTaskIdx].AATSectorRadius;
             else
                 SecRadius = Task[iTaskIdx].AATCircleRadius;
         }

         lk::snprintf(text2, _T("Radius %3.1f%s (%i%s)"),
                   Units::ToDistance(SecRadius), Units::GetDistanceName(),
                   (int) Units::ToAltitude(WayPointList[idx].Altitude),
                   Units::GetAltitudeName());
     }
  }
  return 0;
}

static
int ShowTextEntries(LKSurface& Surface, const RECT& rc, TCHAR (&text1)[MAX_LEN], TCHAR (&text2)[MAX_LEN])
{
  /********************
   * show text
   ********************/
  Surface.SetBackgroundTransparent();
  Surface.SetTextColor(RGB_BLACK);
  Surface.DrawText(rc.right + DLGSCALE(2), DLGSCALE(2), text1);
  int ytext2 = Surface.GetTextHeight(text1);
  Surface.SetTextColor(RGB_DARKBLUE);
  Surface.DrawText(rc.right + DLGSCALE(2), ytext2, text2);
  return 0;
}

static
int BuildLandableText(int idx, double Distance, TCHAR (&text1)[MAX_LEN], TCHAR (&text2)[MAX_LEN])
{
  TCHAR Comment[MAX_COMMENT] = _T("");

  if (WayPointList[idx].Comment != NULL) {
    CopyTruncateString(Comment, MAX_COMMENT, WayPointList[idx].Comment);
  } else {
      lk::strcpy(Comment, TEXT(""));
  }

  if (_tcslen(WayPointList[idx].Freq) > 0) {
      // remove spaces from frequency
      for (int j = 1; j < (CUPSIZE_FREQ); j++)
          if (WayPointList[idx].Freq[CUPSIZE_FREQ - j] == ' ')
              WayPointList[idx].Freq[CUPSIZE_FREQ - j] = '\0';

      if (_tcslen(WayPointList[idx].Freq) > 2)
        lk::snprintf(text1, _T("%s %s"), WayPointList[idx].Name,
                    WayPointList[idx].Freq);
      else
        lk::snprintf(text1, _T("%s"), WayPointList[idx].Name);
  } else {
      if( WayPointList[idx].Style ==  STYLE_THERMAL)
        lk::snprintf(text1, _T("%s: %s"), MsgToken<905>(), WayPointList[idx].Name);
      else
      if (WayPointList[idx].Comment != NULL)
        lk::snprintf(text1, _T("%s %s"), WayPointList[idx].Name, Comment);
      else
        lk::snprintf(text1, _T("%s"), WayPointList[idx].Name);
  }

  if ((WayPointList[idx].RunwayLen >= 10) ||
      (WayPointList[idx].RunwayDir > 0)) {
      lk::snprintf(text2, _T("%3.1f%s (%i%s  %02i/%02i  %i%s)"),
                Units::ToDistance(Distance), Units::GetDistanceName(),
                (int) Units::ToAltitude(WayPointList[idx].Altitude),
                Units::GetAltitudeName(),
                (int) (WayPointList[idx].RunwayDir / 10.0 + 0.5),
                (int) (AngleLimit360(WayPointList[idx].RunwayDir + 180.0) /
                       10.0 + 0.5),
                (int) Units::ToAltitude(WayPointList[idx].RunwayLen),
                Units::GetAltitudeName());
  } else {
      lk::snprintf(text2, _T("%3.1f%s (%i%s) "),
                Units::ToDistance(Distance),
                Units::GetDistanceName(),
                (int) Units::ToAltitude(WayPointList[idx].Altitude),
                Units::GetAltitudeName());
  }
  return 0;
}

void UTF8Pictorial(LKSurface& Surface, const RECT& rc,const TCHAR *Pict ,const LKColor& Color)
{
if (Pict == NULL) return;
  Surface.SetBackgroundTransparent();
  const auto OldFont =  Surface.SelectObject(LK8PanelBigFont);

  Surface.SetTextColor(Color);
  int xtext = Surface.GetTextWidth(Pict);
  Surface.DrawText(rc.left +(rc.right-rc.left-xtext)/2 , DLGSCALE(2), Pict);
  Surface.SelectObject(OldFont);
}

void DrawUTF8FlarmPicto(LKSurface& Surface, const RECT& rc, FLARM_TRAFFIC* pTraf)
{
if (pTraf == NULL) return;
double fFact = fabs(pTraf->Average30s + 5.0) /10.0;
if(fFact > 1.0 ) fFact = 1.0; else
  if(fFact < 0.0 ) fFact = 0.0;
LKColor BaseColor = RGB_GREEN ;
if (IsDithered()) {
  BaseColor = RGB_BLACK;
} else {
  BaseColor = BaseColor.MixColors(RGB_BLUE, fFact);
}

if(pTraf->Status == LKT_GHOST)  UTF8Pictorial( Surface,  rc, MsgToken<2382>() ,BaseColor); else     // _@M2382_ "■"
  if(pTraf->Status == LKT_ZOMBIE) UTF8Pictorial( Surface,  rc,  MsgToken<2383>() ,BaseColor);else      // _@M2383_ "●"
    UTF8Pictorial( Surface,  rc, MsgToken<2384>()  ,BaseColor);    // _@M2384_ "●"
}



static void OnMultiSelectListPaintListItem(WndOwnerDrawFrame * Sender, LKSurface& Surface) {
#define PICTO_WIDTH 50
    Surface.SetTextColor(RGB_BLACK);
    if ((DrawListIndex < std::min(iNO_ELEMENTS, MAX_LIST_ITEMS)) &&(DrawListIndex >= 0)) {
        const ListElement& el = Elements[DrawListIndex];

        static CAirspaceBase airspace_copy;
        PixelRect rc = {
            0, 
            0, 
            DLGSCALE(PICTO_WIDTH), 
            static_cast<PixelScalar>(Sender->GetHeight())
        };

        const CAirspace* pAS = NULL;
        int HorDist, Bearing, VertDist;
        double Distance;
        unsigned int idx = 0;
        TCHAR text1[MAX_LEN] = {TEXT("empty")};
        TCHAR text2[MAX_LEN] = {TEXT("empty")};
        TCHAR Comment[MAX_COMMENT] = {TEXT("")};
        TCHAR Comment1[MAX_COMMENT] = {TEXT("")};
        Surface.SetBkColor(RGB_WHITE);

        switch (el.type) {
            /************************************************************************************************
             * IM_AIRSPACE
             ************************************************************************************************/
        case IM_AIRSPACE:
            pAS = (CAirspace*) el.ptr;
            if (pAS) {
                /***********************************************************************
                 * here we use a local copy of the airspace, only common property exists
                 ***********************************************************************/
                airspace_copy = CAirspaceManager::Instance().GetAirspaceCopy(pAS);

                // airspace type already in name?
                if (_tcsnicmp(airspace_copy.Name(), airspace_copy.TypeName(), _tcslen(airspace_copy.TypeName())) == 0) {
                    lk::snprintf(text1, _T("%s"), airspace_copy.Name()); // yes, take name only
                } else {
                    // fixed strings max. 20 NAME_SIZE 30 => max. 30 char
                    lk::snprintf(text1, _T("%s %s"), airspace_copy.TypeName(), airspace_copy.Name());
                }

                CAirspaceManager::Instance().GetSimpleAirspaceAltText(Comment, sizeof (Comment) / sizeof (Comment[0]), airspace_copy.Top());
                CAirspaceManager::Instance().GetSimpleAirspaceAltText(Comment1, sizeof (Comment1) / sizeof (Comment1[0]), airspace_copy.Base());

                CAirspaceManager::Instance().AirspaceCalculateDistance((CAirspace*) pAS, &HorDist, &Bearing, &VertDist);
                lk::snprintf(text2, _T("%3.1f%s (%s - %s)"),
                          Units::ToDistance(HorDist),
                          Units::GetDistanceName(),
                          Comment1, Comment); //8 + 8+3   21

                /****************************************************************
                 * for drawing the airspace pictorial, we need the original data.
                 * copy contain only base class property, not geo data,
                 * original data are shared ressources !
                 * for that we need to grant all called methods are thread safe
                 ****************************************************************/
                ShowTextEntries(Surface, rc,  text1, text2);
                pAS->DrawPicto(Surface, rc);

            }
            break;

#ifdef  FLARM_MS
            /************************************************************************************************
             * IM_FLARM
             ************************************************************************************************/
        case IM_FLARM:
            LKASSERT(el.iIdx  < FLARM_MAX_TRAFFIC);
            LKASSERT(el.iIdx  >= 0);
            FLARM_TRAFFIC Target;
            LockFlightData();
              memcpy( &Target, &GPS_INFO.FLARM_Traffic[el.iIdx], sizeof(     FLARM_TRAFFIC));
            UnlockFlightData();

            BuildFLARMText(&Target,text1,text2);
            ShowTextEntries(Surface, rc,  text1, text2);

#ifdef FLARM_PICTO_THREADSAFE
            MapWindow::DrawFlarmPicto(Surface, rc, &Target);   // draw MAP icons
#else
            DrawUTF8FlarmPicto(Surface, rc, &Target);          // use alternate UTF8 icons
#endif
            break;
#endif // FLARM_MS

#ifdef WEATHERST_MS
            /************************************************************************************************
             * IM_WEATHERST
             ************************************************************************************************/
        case IM_WEATHERST:
            LKASSERT(el.iIdx  < MAXFANETWEATHER);
            LKASSERT(el.iIdx  >= 0);
            FANET_WEATHER Station;
            TCHAR StationName[MAXFANETNAME+1];
            StationName[0] = 0; //zero-termination of String;
            //int stationIndex;
            //stationIndex = -1;
            LockFlightData();
                memcpy( &Station, &GPS_INFO.FANET_Weather[el.iIdx], sizeof(FANET_WEATHER));
                GetFanetName(Station.ID, GPS_INFO, StationName);
            UnlockFlightData();
            BuildWEATHERText(&Station,text1,text2,StationName);
            ShowTextEntries(Surface, rc,  text1, text2);

            MapWindow::DrawWeatherStPicto(Surface, rc, &Station);   // draw MAP icons

            break;

#endif //  WEATHERST_MS

#ifdef TEAM_CODE_MS
            /************************************************************************************************
             * IM_TEAM
             ************************************************************************************************/
        case IM_TEAM:
          lk::snprintf(text1, _T("%s:"),MsgToken<700>()); //_@M700_ "Team code"
          lk::snprintf(text2, _T("%s"), CALCULATED_INFO.OwnTeamCode );
            ShowTextEntries(Surface, rc,  text1, text2);
            if(Appearance.UTF8Pictorials)
              UTF8Pictorial( Surface,  rc, MsgToken<2380>(), RGB_VDARKRED);  // _@M2380_ "⚑"
            else
              UTF8Pictorial( Surface,  rc, _T("@"), RGB_VDARKRED);

            break;
#endif
#ifdef ORACLE_MS
            /************************************************************************************************
             * IM_ORACLE
             ************************************************************************************************/
        case IM_ORACLE:
          lk::snprintf(text1, _T("%s"), MsgToken<2058>()); //_@M2058_ "Oracle"
            if(el.iIdx >= 0)
              lk::snprintf(text2, _T("%s: %s"), MsgToken<456>(), WayPointList[el.iIdx].Name);// _@M456_ "Near"
            else
              lk::snprintf(text2, _T("%s"), MsgToken<1690>()); //_@M1690_ "THE LK8000 ORACLE"
            ShowTextEntries(Surface, rc,  text1, text2);
            if(Appearance.UTF8Pictorials)
              UTF8Pictorial( Surface,  rc, MsgToken<2381>(),RGB_BLUE);  // _@M2381_ "♕"
            else
              UTF8Pictorial( Surface,  rc, _T("?"),RGB_BLUE);
            break;
#endif
#ifdef OWN_POS_MS
            /************************************************************************************************
             * IM_OWN_POS
             ************************************************************************************************/
          case IM_OWN_POS:
            lk::snprintf(text1, _T("%s [%s]"), AircraftRego_Config,AircraftType_Config);
             if (ISPARAGLIDER || ISCAR)
             {
               int utmzone; char utmchar;
               double easting, northing;
               LatLonToUtmWGS84 ( utmzone, utmchar, easting, northing, GPS_INFO.Latitude, GPS_INFO.Longitude );
               lk::snprintf(text2, _T("UTM %d%c  %.0f  %.0f"), utmzone, utmchar, easting, northing);
             }
             else
             {
               LockFlightData();
               Units::CoordinateToString( GPS_INFO.Longitude,GPS_INFO.Latitude, Comment, sizeof(text2)-1);
               lk::snprintf(text2, _T("%s %6.0f%s"),Comment, Units::To(unFeet, GPS_INFO.Altitude), Units::GetName(unFeet));
               UnlockFlightData();
             }
             ShowTextEntries(Surface, rc,  text1, text2);
             MapWindow::DrawAircraft(Surface, (POINT) { (rc.right-rc.left)/2,(rc.bottom-rc.top)/2} );
          break;
#endif // OWN_POS_MS
            /************************************************************************************************
             * IM_WAYPOINT
             ************************************************************************************************/
        case IM_WAYPOINT:
            idx = -1;

            if(ValidWayPointFast(el.iIdx)) {
                idx = el.iIdx;
            }

            assert(idx < WayPointList.size());
            if(idx < WayPointList.size())
            {
              if (WayPointList[idx].Comment != NULL) {
                CopyTruncateString(Comment, MAX_COMMENT, WayPointList[idx].Comment);
              } else {
                  lk::strcpy(Comment, TEXT(""));
              }
              DistanceBearing(GPS_INFO.Latitude, GPS_INFO.Longitude, WayPointList[idx].Latitude,
                              WayPointList[idx].Longitude, &Distance, NULL);
              BuildLandableText(idx, Distance,text1,text2);
              ShowTextEntries(Surface, rc,  text1, text2);
              if (WayPointCalc[idx].IsLandable) {
                  MapWindow::DrawRunway(Surface, &WayPointList[idx], rc, nullptr, 1.5, true);
              }// waypoint isLandable
              else {
                  MapWindow::DrawWaypointPicto(Surface, rc, &WayPointList[idx]);
              }

            }
            break;
            /************************************************************************************************
             * IM_TASK
             ************************************************************************************************/
        case IM_TASK_PT:
            {
              idx = -1;
              LockTaskData(); // protect from external task changes
              int iTaskIdx = el.iIdx;
              if(ValidTaskPointFast(iTaskIdx))
              {
                idx = Task[iTaskIdx].Index;
              }
              assert(idx < WayPointList.size());
              if(idx < WayPointList.size())
              {
                BuildTaskPointText( iTaskIdx,  text1, text2);
                ShowTextEntries(Surface, rc,  text1, text2);
                MapWindow::DrawTaskPicto(Surface, iTaskIdx, rc);
              }
              UnlockTaskData(); // protect from external task changes
            }
            break;
        }


    }
}

static void OnMultiSelectListListEnter(WindowControl * Sender,
                                       WndListFrame::ListInfo_t *ListInfo) {
    (void) Sender;

    ItemIndex = ListInfo->ItemIndex + ListInfo->ScrollIndex;
    if (ItemIndex >= iNO_ELEMENTS) {
        ItemIndex = iNO_ELEMENTS - 1;
    }
    if (ItemIndex >= 0) {
      if(Sender) {
        WndForm * pForm = Sender->GetParentWndForm();
        if(pForm) {
          pForm->SetModalResult(mrOK);
        }
      }
    }


    if ((ItemIndex >= 0) && (ItemIndex < iNO_ELEMENTS)) {
        dlgAddMultiSelectListDetailsDialog(ItemIndex);
    }

}

static void OnEnterClicked(WndButton* pWnd) {
    (void)pWnd;

    if (ItemIndex >= iNO_ELEMENTS) {
        ItemIndex = iNO_ELEMENTS - 1;
    }
    if (ItemIndex >= 0) {
      if(pWnd) {
        WndForm * pForm = pWnd->GetParentWndForm();
        if(pForm) {
          pForm->SetModalResult(mrOK);
        }
      }
    }

    if ((ItemIndex >= 0) && (ItemIndex < iNO_ELEMENTS)) {
        dlgAddMultiSelectListDetailsDialog(ItemIndex);
    }

}

static void OnMultiSelectListListInfo(WndListFrame * Sender, WndListFrame::ListInfo_t *ListInfo) {

    if (ListInfo->DrawIndex == -1) {
        ListInfo->ItemCount = iNO_ELEMENTS;
    } else {
        DrawListIndex = ListInfo->DrawIndex + ListInfo->ScrollIndex;
        ItemIndex = ListInfo->ItemIndex + ListInfo->ScrollIndex;
    }

}

static void OnCloseClicked(WndButton* pWnd) {
  ItemIndex = -1;
  if(pWnd) {
    WndForm * pForm = pWnd->GetParentWndForm();
    if(pForm) {
      pForm->SetModalResult(mrCancel);
    }
  }
}




static CallBackTableEntry_t CallBackTable[] = {
    OnPaintCallbackEntry(OnMultiSelectListPaintListItem),
    OnListCallbackEntry(OnMultiSelectListListInfo),
    ClickNotifyCallbackEntry(OnCloseClicked),
    ClickNotifyCallbackEntry(OnUpClicked),
    ClickNotifyCallbackEntry(OnEnterClicked),
    ClickNotifyCallbackEntry(OnDownClicked),
    EndCallBackEntry()
};

ListElement* dlgMultiSelectListShowModal(void) {

    ItemIndex = -1;

    if (iNO_ELEMENTS == 0) {

        return NULL;
    }

    wf = dlgLoadFromXML(CallBackTable, ScreenLandscape ? IDR_XML_MULTISELECTLIST_L : IDR_XML_MULTISELECTLIST_P);

    if (!wf) return NULL;

    wf->SetTimerNotify(1000, OnTimer);
    wMultiSelectListList = wf->FindByName<WndListFrame>(TEXT("frmMultiSelectListList"));
    LKASSERT(wMultiSelectListList != NULL);
    wMultiSelectListList->SetBorderKind(BORDERLEFT);
    wMultiSelectListList->SetEnterCallback(OnMultiSelectListListEnter);

    wMultiSelectListListEntry = wf->FindByName<WndOwnerDrawFrame>(TEXT("frmMultiSelectListListEntry"));
    if(wMultiSelectListListEntry) {
        /*
         * control height must contains 2 text Line 
         * Check and update Height if necessary
         */
        LKWindowSurface windowSurface(*main_window);
        LKBitmapSurface tmpSurface(windowSurface, 1, 1);

        const auto oldFont = tmpSurface.SelectObject(wMultiSelectListListEntry->GetFont());
        const int minHeight = 2 * tmpSurface.GetTextHeight(_T("dp")) + 2 * DLGSCALE(2);
        tmpSurface.SelectObject(oldFont);
        const int wHeight = wMultiSelectListListEntry->GetHeight();
        if(minHeight > wHeight) {
            wMultiSelectListListEntry->SetHeight(minHeight);
        }
        wMultiSelectListListEntry->SetCanFocus(true);
    } else LKASSERT(0);

    UpdateList();

    wf->ShowModal();

    delete wf;

    wf = NULL;

    iNO_ELEMENTS = 0;

    NoAirfields = 0;
    NoOutlands = 0;
    NoWaypoints = 0;
    NoAirspace = 0;
    NoTaskPoints = 0;
#ifdef FLARM_MS
    NoFlarm = 0;
#endif
#ifdef WEATHERST_MS
    NoWeatherSt = 0;
#endif
    return pResult;
}
