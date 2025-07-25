/***********************************************************************
**
**   windmeasurementlist.h
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
**   $Id: windmeasurementlist.h,v 1.1 2011/12/21 10:35:29 root Exp root $
**
***********************************************************************/

#ifndef WINDMEASUREMENTLIST_H
#define WINDMEASUREMENTLIST_H

/**The WindMeasurementList is a list that can contain and process windmeasurements.
  *@author André Somers
  */
#include <memory>
#include <optional>
#include "Math/Point2D.hpp"
using Vector = Point2D<double>;


struct WindMeasurement {
  Vector vector;
  int quality;
  long time;
  double altitude;
};

#define MAX_MEASUREMENTS 200
//maximum number of windmeasurements in the list. No idea what a sensible value would be...

class WindMeasurementList final {
 public:
  WindMeasurementList() = default;

  /**
   * Returns the weighted mean windvector over the stored values, or 0
   * if no valid vector could be calculated (for instance: too little or
   * too low quality data).
   */
  std::optional<Vector> getWind(double Time, double alt) const;

  /** Adds the windvector vector with quality quality to the list. */
  void addMeasurement(double Time, Vector vector, double alt, int quality);

 protected:
  std::unique_ptr<const WindMeasurement> measurementlist[MAX_MEASUREMENTS];
  unsigned int nummeasurementlist = 0;

  /**
   * getLeastImportantItem is called to identify the item that should be
   * removed if the list is too full. Reimplemented from LimitedList.
   */
  unsigned int getLeastImportantItem(double Time);
};

#endif
