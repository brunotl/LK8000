/***********************************************************************
**
**   windanalyser.h
**
**   This file is part of Cumulus.
**
************************************************************************
**
**   Copyright (c):  2002 by André Somers
**
**   This file is distributed under the terms of the General Public
**   Licence. See the file COPYING for more information.
**
**   $Id: windanalyser.h,v 1.1 2011/12/21 10:35:29 root Exp root $
**
***********************************************************************/

#ifndef WINDANALYSER_H
#define WINDANALYSER_H

#include "windstore.h"

/**The windanalyser analyses the list of flightsamples looking for windspeed and direction.
  *@author André Somers
  */


struct WindSample {
  Vector v;
  double t;
  double mag;
};

class WindAnalyser final {
 public:
  WindAnalyser() = default;

  /**
   * Called if the flightmode changes
   */
  void slot_newFlightMode(NMEA_INFO* nmeaInfo, DERIVED_INFO* derivedInfo, bool left, int);
  /**
   * Called if a new sample is available in the samplelist.
   */
  void slot_newSample(NMEA_INFO* nmeaInfo, DERIVED_INFO* derivedInfo);

  // used to update output if altitude changes
  void slot_Altitude(NMEA_INFO* nmeaInfo, DERIVED_INFO* derivedInfo);

  void slot_newEstimate(NMEA_INFO* nmeaInfo, DERIVED_INFO* derivedInfo, Vector v, int quality);

  Vector getWind(double Time, double h) const {
    return windstore.getWind(Time, h);
  }

 private:               // Private attributes
  int circleCount = 0;  // we are counting the number of circles, the first onces are probably not very round
  bool active = false;  // active is set to true or false by the slot_newFlightMode slot
  int circleDeg = 0;
  int lastHeading = 0;
  Vector minVector = {};
  Vector maxVector = {};
  bool curModeOK = false;

  std::vector<WindSample> windsamples;

  WindStore windstore;

 private:  // Private memberfunctions
  void _calcWind(NMEA_INFO* nmeaInfo, DERIVED_INFO* derivedInfo);
};

#endif
