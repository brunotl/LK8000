/***********************************************************************
**
**   windmeasurementlist.cpp
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

#include "externs.h"
#include "utils/array_adaptor.h"
#include "windmeasurementlist.h"

/**
  * Returns the weighted mean windvector over the stored values, or 0
  * if no valid vector could be calculated (for instance: too little or
  * too low quality data).
  */
std::optional<Vector> WindMeasurementList::getWind(double Time, double alt) const {
  //relative weight for each factor
  #define REL_FACTOR_QUALITY 100
  #define REL_FACTOR_ALTITUDE 100
  #define REL_FACTOR_TIME 200
  #define TIME_RANGE 36 // one hour

  constexpr int altRange  = 1000; //conf->getWindAltitudeRange();
  constexpr int timeRange = TIME_RANGE*100; //conf->getWindTimeRange();

  unsigned int total_quality=0;

  Vector result = {0, 0};
  int now= (int)(Time);

  double override_time = 1.1;
  bool overridden = false;

  for (const auto& m : make_array(measurementlist, nummeasurementlist)) {
    double altdiff = (alt - m->altitude) * 1.0 / altRange;
    double timediff = fabs((double)(now - m->time) / timeRange);

    if ((fabs(altdiff) < 1.0) && (timediff < 1.0)) {
      unsigned q_quality = min(5, m->quality) * REL_FACTOR_QUALITY / 5;
      // measurement quality

      unsigned a_quality = iround(((2.0 / (altdiff * altdiff + 1.0)) - 1.0) * REL_FACTOR_ALTITUDE);

      // factor in altitude difference between current altitude and
      // measurement.  Maximum alt difference is 1000 m.

      constexpr double k = 0.0025;

      unsigned t_quality = iround(k * (1.0 - timediff) / (timediff * timediff + k) * REL_FACTOR_TIME);
      // factor in timedifference. Maximum difference is 1 hours.

      if (m->quality == 6) {
        if (timediff < override_time) {
          // over-ride happened, so re-set accumulator
          override_time = timediff;
          total_quality = 0;
          result = { 0, 0 };
          overridden = true;
        }
        else {
          // this isn't the latest over-ride or obtained fix, so ignore
          continue;
        }
      }
      else {
        if (timediff < override_time) {
          // a more recent fix was obtained than the over-ride, so start using
          // that one
          override_time = timediff;
          if (overridden) {
            // re-set accumulators
            overridden = false;
            total_quality = 0;
            result = { 0, 0 };
          }
        }
      }
      unsigned quality = q_quality * (a_quality * t_quality);
      result += m->vector * quality;
      total_quality += quality;
    }
  }

  if (total_quality > 0) {
    return result / total_quality;
  }

  return {};
}


/** Adds the windvector vector with quality quality to the list. */
void WindMeasurementList::addMeasurement(double Time,
                                         Vector vector, double alt, int quality){
  uint index;
  if (nummeasurementlist == MAX_MEASUREMENTS) {
    index = getLeastImportantItem(Time);
  }
  else {
    index = nummeasurementlist++;
  }
  measurementlist[index] = std::make_unique<WindMeasurement>(WindMeasurement{vector, quality, static_cast<long>(Time), alt});
}

/**
 * getLeastImportantItem is called to identify the item that should be
 * removed if the list is too full. Reimplemented from LimitedList.
 */
uint WindMeasurementList::getLeastImportantItem(double Time) {

  int maxscore=0;

  unsigned int founditem = nummeasurementlist - 1;
  for (int i = founditem; i >= 0; i--) {
    //Calculate the score of this item. The item with the highest
    //score is the least important one.  We may need to adjust the
    //proportion of the quality and the elapsed time. Currently, one
    //quality-point (scale: 1 to 5) is equal to 10 minutes.

    int score = 600 * (6 - measurementlist[i]->quality);
    score += (int)(Time - (double)measurementlist[i]->time);
    if (score > maxscore) {
      maxscore = score;
      founditem = i;
    }
  }
  return founditem;
}
