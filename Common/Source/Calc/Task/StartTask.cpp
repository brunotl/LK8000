/*
   LK8000 Tactical Flight Computer -  WWW.LK8000.IT
   Released under GNU/GPL License v.2 or later
   See CREDITS.TXT file for authors and copyrights

   $Id$
*/

#include "externs.h"
#include "CalcTask.h"
#include "AATDistance.h"
#include "Calc/Task/TimeGates.h"

void StartTask(NMEA_INFO* Basic, DERIVED_INFO* Calculated, const bool do_advance, const bool do_announce) {

  double TaskStartTime = Basic->Time;
  try {
    if (UseGates() && HaveGates(TaskStartTime)) {
      const int gateTime = OpenGateTime();
      if ( gateTime > 0 ) {
        TaskStartTime = gateTime - GetUTCOffset();
      }
    }
  }
  catch(std::exception& e) {
    TestLog(_T("exception : %s"), to_string(e.what()).c_str());
  }


  Calculated->ValidFinish = false;
  Calculated->TaskStartTime = TaskStartTime ;
  Calculated->TaskStartSpeed = Basic->Speed;
  Calculated->TaskStartAltitude = Calculated->NavAltitude;
  Calculated->LegStartTime = Basic->Time;
  flightstats.LegStartTime[0] = Basic->Time;
  flightstats.LegStartTime[1] = Basic->Time;

  Calculated->CruiseStartLat = Basic->Latitude;
  Calculated->CruiseStartLong = Basic->Longitude;
  Calculated->CruiseStartAlt = Calculated->NavAltitude;
  Calculated->CruiseStartTime = Basic->Time;

  aatdistance.Reset();

  // JMW TODO accuracy: Get time from aatdistance module since this is
  // more accurate

  // JMW clear thermal climb average on task start
  flightstats.ThermalAverage.Reset();
  flightstats.Task_Speed.Reset();
  Calculated->AverageThermal = 0;
  Calculated->WaypointBearing=0;

  // JMW reset time cruising/time circling stats on task start
  Calculated->timeCircling = 0;
  Calculated->timeCruising = 0;
  Calculated->TotalHeightClimb = 0;

  if (do_announce) {
    AnnounceWayPointSwitch(Calculated, do_advance);
  } else {
    if (do_advance) {
      ActiveTaskPoint=1;
      SelectedWaypoint = TASKINDEX; 
    }
  }
}
