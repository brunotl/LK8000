/***********************************************************************
**
**   windstore.cpp
**
**   This file is part of Cumulus
**
************************************************************************
**
**   Copyright (c):  2002 by André Somers
**
**   This file is distributed under the terms of the General Public
**   Licence. See the file COPYING for more information.
**
**
***********************************************************************/
#include "Compiler.h"
#include "options.h"
#include "windstore.h"
#include "NMEA/Info.h"
#include "NMEA/Derived.h"
#include "Defines.h"
#include "MathFunctions.h"
/**
  * Called with new measurements. The quality is a measure for how
  * good the measurement is. Higher quality measurements are more
  * important in the end result and stay in the store longer.
  */
void WindStore::slot_measurement(NMEA_INFO *nmeaInfo,
                                 DERIVED_INFO *derivedInfo,
                                 Vector windvector, int quality){
  updated = true;
  windlist.addMeasurement(nmeaInfo->Time, windvector, nmeaInfo->Altitude, quality);
  //we may have a new wind value, so make sure it's emitted if needed!
  recalculateWind(nmeaInfo, derivedInfo);
}


/**
  * Called if the altitude changes.
  * Determines where measurements are stored and may result in a
  * newWind signal.
  */

void WindStore::slot_Altitude(NMEA_INFO *nmeaInfo,
                              DERIVED_INFO *derivedInfo){
  if ((fabs(nmeaInfo->Altitude - _lastAltitude) > 100.0) || (updated)) {
    // only recalculate if there is a significant change
    recalculateWind(nmeaInfo, derivedInfo);

    updated = false;
    _lastAltitude=nmeaInfo->Altitude;
  }
}


Vector WindStore::getWind(double Time, double h) const {
  return windlist.getWind(Time, h).value_or(Vector{0., 0.});
}

/** Recalculates the wind from the stored measurements.
  * May result in a newWind signal. */

void WindStore::recalculateWind(NMEA_INFO *nmeaInfo,
                                DERIVED_INFO *derivedInfo) {

  auto CurWind = windlist.getWind(nmeaInfo->Time, nmeaInfo->Altitude);
  if (CurWind) {
    Vector diff = (*CurWind) - _lastWind;
    if ((fabs(diff.x) > 1.0) || (fabs(diff.y) > 1.0) || updated) {
      _lastWind = *CurWind;

      updated = false;
      _lastAltitude = nmeaInfo->Altitude;

      newWind(nmeaInfo, derivedInfo, CurWind.value());
    }
  }  // otherwise, don't change anything
}


void WindStore::newWind(NMEA_INFO *nmeaInfo, DERIVED_INFO *derivedInfo,
                        Vector &wind) {

  double mag = Length(wind);
  if (mag < 30) {  // limit to reasonable values
    derivedInfo->WindSpeed = mag;

    if (wind == Vector{0., 0.} ) {
      derivedInfo->WindBearing = 0;
    }
    else {
      derivedInfo->WindBearing = AngleLimit360(atan2(wind.y, wind.x) * RAD_TO_DEG);
    }
  }
  else {
    // TODO code: give warning, wind estimate bogus or very strong!
  }
}
