/*
   LK8000 Tactical Flight Computer -  WWW.LK8000.IT
   Released under GNU/GPL License v.2
   See CREDITS.TXT file for authors and copyrights

   $Id$
*/

#include "externs.h"
#include "CriticalSection.h"
#include "McReady.h"
#include "NavFunctions.h"

static void simpleETE(NMEA_INFO *Basic, DERIVED_INFO *Calculated,
                      WPCALC &wpt_calc) {
  if (Basic->Speed < 1 || !Calculated->Flying || Calculated->Circling) {
    return;
  }
  wpt_calc.NextETE = wpt_calc.Distance / Basic->Speed;
}

// This is also called by DoNearest and it is overwriting AltitudeRequired
double CalculateWaypointArrivalAltitude(NMEA_INFO *Basic,
                                        DERIVED_INFO *Calculated, int i) {

  CScopeLock Lock(LockTaskData, UnlockTaskData);

  if (!ValidWayPointFast(i)) {
    assert(false); // BUG ?
    return 0.;
  }

  const WAYPOINT &wpt = WayPointList[i];
  WPCALC &wpt_calc = WayPointCalc[i];

  double wDistance, wBearing;
  DistanceBearing(Basic->Latitude, Basic->Longitude, wpt.Latitude,
                  wpt.Longitude, &wDistance, &wBearing);

  wpt_calc.Distance = wDistance;
  wpt_calc.Bearing = wBearing;

  if (ISCAR) {
    simpleETE(Basic, Calculated, wpt_calc);
    return (Basic->Altitude - wpt.Altitude);
  }

  const double safetyaltitudearrival = GetSafetyAltitude(i);
  const double defaultMC = GetMacCready(i, GMC_DEFAULT);

  const double HeightReqd = GlidePolar::MacCreadyAltitude(
                      defaultMC, 
                      wDistance, wBearing, 
                      Calculated->WindSpeed, Calculated->WindBearing, 
                      nullptr, nullptr, true, 
                      &wpt_calc.NextETE);

  // we should build a function for this since it is used also in lkcalc
  wpt_calc.AltReqd[AltArrivMode] = HeightReqd + safetyaltitudearrival +
                                   wpt.Altitude - Calculated->EnergyHeight;

  // Alt Arrival is Alt Required - current Altitude
  wpt_calc.AltArriv[AltArrivMode] =
      Calculated->NavAltitude - wpt_calc.AltReqd[AltArrivMode];

  if (wpt_calc.AltArriv[AltArrivMode] < 0.) {
    // we can't reach the turnpoint without climbing.
    // so, we need to calculate time needed to reach altrequired...
    if (defaultMC > 0.) {
      // ... using  MC value for meam climb speed 
      const double AltToClimb = (-1 * wpt_calc.AltArriv[AltArrivMode]);
      wpt_calc.NextETE += AltToClimb / defaultMC;
    } else {
      // ... but with MC 0, we can't calculate NextETE...
      wpt_calc.NextETE = 0;
    }
  }

  // for GA recalculate simple ETE
  if (ISGAAIRCRAFT) {
    simpleETE(Basic, Calculated, wpt_calc);
  }

  return (wpt_calc.AltArriv[AltArrivMode]);
}
