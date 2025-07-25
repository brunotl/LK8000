/***********************************************************************
**
**   windanalyser.cpp
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

/*
   LK8000 Tactical Flight Computer -  WWW.LK8000.IT
   Released under GNU/GPL License v.2 or later
   See CREDITS.TXT file for authors and copyrights

*/

#include "externs.h"

#include "windanalyser.h"


/*
  About Windanalysation

  Currently, the wind is being analyzed by finding the minimum and the maximum
  groundspeeds measured while flying a circle. The direction of the wind is taken
  to be the direction in wich the speed reaches it's maximum value, the speed
  is half the difference between the maximum and the minimum speeds measured.
  A quality parameter, based on the number of circles allready flown (the first
  circles are taken to be less accurate) and the angle between the headings at
  minimum and maximum speeds, is calculated in order to be able to weigh the
  resulting measurement.

  There are other options for determining the windspeed. You could for instance
  add all the vectors in a circle, and take the resuling vector as the windspeed.
  This is a more complex method, but because it is based on more heading/speed
  measurements by the GPS, it is probably more accurate. If equiped with
  instruments that pass along airspeed, the calculations can be compensated for
  changes in airspeed, resulting in better measurements. We are now assuming
  the pilot flies in perfect circles with constant airspeed, wich is of course
  not a safe assumption.
  The quality indication we are calculation can also be approched differently,
  by calculating how constant the speed in the circle would be if corrected for
  the windspeed we just derived. The more constant, the better. This is again
  more CPU intensive, but may produce better results.

  Some of the errors made here will be averaged-out by the WindStore, wich keeps
  a number of windmeasurements and calculates a weighted average based on quality.
*/

/** Called if a new sample is available in the samplelist. */
void WindAnalyser::slot_newSample(NMEA_INFO *nmeaInfo,
                                  DERIVED_INFO *derivedInfo) {
  if (!active) {
    return;  // only work if we are in active mode
  }

  bool fullCircle=false;

  // circle detection
  if (lastHeading != nmeaInfo->TrackBearing) {
    circleDeg += abs(AngleLimit180(nmeaInfo->TrackBearing - lastHeading));
    lastHeading = nmeaInfo->TrackBearing;
  }

  if (circleDeg >= 360) {
    // full circle made!

    fullCircle = true;
    circleDeg = 0;
    circleCount++;  // increase the number of circles flown (used
    // to determine the quality)
  }

  Vector curVector = {
    nmeaInfo->Speed * cos(nmeaInfo->TrackBearing * 3.14159 / 180.0),
    nmeaInfo->Speed * sin(nmeaInfo->TrackBearing * 3.14159 / 180.0)
  };

  if (windsamples.empty()) {
    minVector = maxVector = curVector;
  } 
  else {
    if (nmeaInfo->Speed < Length(minVector)) {
      minVector = curVector;
    }
    if (nmeaInfo->Speed > Length(maxVector)) {
      maxVector = curVector;
    }
  }

  windsamples.push_back({curVector, nmeaInfo->Time, Length(curVector)});

  if (fullCircle) { //we have completed a full circle!

    _calcWind(nmeaInfo, derivedInfo);    //calculate the wind for this circle
    fullCircle=false;

    // should set each vector to average
    Vector v = (maxVector - minVector) / 2;

    minVector = v;
    maxVector = v;

    windsamples.clear();

    //no need to reset fullCircle, it will automaticly be reset in the next itteration.
  }
  windstore.slot_Altitude(nmeaInfo, derivedInfo);
}


void WindAnalyser::slot_Altitude(NMEA_INFO *nmeaInfo,
                                 DERIVED_INFO *derivedInfo) {
  windstore.slot_Altitude(nmeaInfo, derivedInfo);
}

/** Called if the flightmode changes */
void WindAnalyser::slot_newFlightMode(NMEA_INFO *nmeaInfo,
                                      DERIVED_INFO *derivedInfo,
                                      bool left, int marker) {
  active = false;   // we are inactive by default
  circleCount = 0;  // reset the circlecounter for each flightmode
                    // change. The important thing to measure is the
                    // number of turns in this thermal only.
  circleDeg = 0;

  if (derivedInfo->Circling) {
    curModeOK = left;
  }
  else {
    curModeOK = false;
    return;  // ok, so we are not circling. Exit function.
  }

  // initialize analyser-parameters
  active = true;
  windsamples.clear();
}

void WindAnalyser::_calcWind(NMEA_INFO* nmeaInfo, DERIVED_INFO* derivedInfo) {
  if (windsamples.empty()) {
    return;
  }

  const double circle_time = (windsamples.back().t - windsamples.front().t);

  // reject if circle time greater than 50 second
  if (circle_time > 50.0) {
    return;
  }

  const int numwindsamples = windsamples.size();

  // reject if average time step greater than 2.0 seconds
  if ((circle_time / (numwindsamples - 1)) > 2.0) {
    return;
  }

  // find average magnitude
  double av = 0;
  for (int i = 0; i < numwindsamples; i++) {
    av += windsamples[i].mag;
  }
  av /= numwindsamples;

  // find zero time for times above average
  double rthismax = 0;
  double rthismin = 0;
  int jmax= -1;
  int jmin= -1;

  for (int j = 0; j < numwindsamples; j++) {
    double rthisp = 0;

    for (int i = 0; i < numwindsamples; i++) {
      if (i == j) {
        continue;
      }
      const int ithis = (i + j) % numwindsamples;
      int idiff = i;
      if (idiff > numwindsamples / 2) {
        idiff = numwindsamples - idiff;
      }
      rthisp += (windsamples[ithis].mag) * idiff;
    }
    if ((rthisp < rthismax) || (jmax == -1)) {
      rthismax = rthisp;
      jmax = j;
    }
    if ((rthisp > rthismin) || (jmin == -1)) {
      rthismin = rthisp;
      jmin = j;
    }
  }

  if(jmin<0 || jmax<0) {
      return;
  }
  // jmax is the point where most wind samples are below
  // jmin is the point where most wind samples are above

  maxVector = windsamples[jmax].v;
  minVector = windsamples[jmin].v;

  // attempt to fit cycloid
  const double mag = 0.5 * (windsamples[jmax].mag - windsamples[jmin].mag);
  double rthis = 0;
  for (int i = 0; i < numwindsamples; i++) {
    const double phase = ((i + jmax) % numwindsamples) * 3.141592 * 2.0 / numwindsamples;
    const double wx = cos(phase) * av + mag;
    const double wy = sin(phase) * av;
    const double cmag = sqrt(wx * wx + wy * wy) - windsamples[i].mag;
    rthis += cmag * cmag;
  }
  rthis /= numwindsamples;
  BUGSTOP_LKASSERT(rthis >= 0);
  if (rthis < 0) {
    return;  // UNMANAGED
  }
  rthis = sqrt(rthis);

  int quality;

  if (mag > 1) {
    quality = 5 - iround(rthis / mag * 3);
  }
  else {
    quality = 5 - iround(rthis);
  }

  if (circleCount < 3) {
    quality--;
  }
  if (circleCount < 2) {
    quality--;
  }
  if (circleCount < 1) {
    return;
  }

  if (quality < 1) {
    return;  // measurement quality too low
  }

  quality = min(quality, 5);  // 5 is maximum quality, make sure we honor that.

  BUGSTOP_LKASSERT(windsamples[jmax].mag != 0);
  if (windsamples[jmax].mag == 0) {
    return;
  }

  const Vector a = {
    -mag * maxVector.x / windsamples[jmax].mag, 
    -mag * maxVector.y / windsamples[jmax].mag
  };

  if (a.x * a.x + a.y * a.y < 30 * 30) {
    // limit to reasonable values (60 knots), reject otherwise
    slot_newEstimate(nmeaInfo, derivedInfo, a, quality);
  }
}

void WindAnalyser::slot_newEstimate(NMEA_INFO* nmeaInfo,
                                    DERIVED_INFO* derivedInfo,
                                    Vector a, int quality) {
  windstore.slot_measurement(nmeaInfo, derivedInfo, a, quality);
}
