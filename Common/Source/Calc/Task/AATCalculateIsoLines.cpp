/*
   LK8000 Tactical Flight Computer -  WWW.LK8000.IT
   Released under GNU/GPL License v.2 or later
   See CREDITS.TXT file for authors and copyrights

   $Id$
*/

#include "externs.h"
#include "CalcTask.h"
#include "Calc/Task/task_zone.h"


void CalculateAATIsoLines() {
  const std::lock_guard lock(CritSec_TaskData);

  constexpr double stepsize = 25.0;

  if(gTaskType != task_type_t::AAT) {
    return;
  }

  if (!ValidTaskPointFast(ActiveTaskPoint)) {
    return;
  }

  for (int i = 0; i < ActiveTaskPoint; i++) {
    std::ranges::fill(TaskStats[i].IsoLine_valid, false);
  }

  for (int i = ActiveTaskPoint; ValidTaskPointFast(i + 1); i++) {
    std::ranges::fill(TaskStats[i].IsoLine_valid, false);

    if (i == 0) {
      continue;
    }

    auto target = GetTurnpointTarget(i);
    const auto zone_data = task::get_zone_data(i);
    const double max_radius = task::get_max_radius(zone_data);

    const double delta = max_radius * 2.4 / (MAXISOLINES);
    bool left = false;

    // insert start point
    TaskStats[i].IsoLine_Geo[0] = target;
    TaskStats[i].IsoLine_valid[0] = true;
    int j = 1;
    while (j < MAXISOLINES) {

      // Distance value at the current point.
      double dist_0 =
          DoubleLegDistance(i, target);

      // Estimate the local gradient of DoubleLegDistance() by evaluating
      // the function one step to the north and one step to the east.
      auto north = target.Direct(0, stepsize);
      double dist_north =
          DoubleLegDistance(i, north);

      auto east = target.Direct(90, stepsize);
      double dist_east = 
          DoubleLegDistance(i, east);

      // Compute the gradient direction. Rotating it by 90° gives the
      // tangent to the equal-distance contour (isoline).
      const double dx = dist_east - dist_0;
      const double dy = dist_north - dist_0;
      double angle = AngleLimit360(RAD_TO_DEG * atan2(dx, dy) + 90);

      // Walk the contour in the opposite direction when tracing
      // the second half of the isoline.
      if (left) {
        angle += 180;
      }

      // Advance one step along the contour.
      target = target.Direct(angle, delta);

      bool in_sector = task::in_turn_sector(target, zone_data);
      if (in_sector) {
        TaskStats[i].IsoLine_Geo[j] = target;
        TaskStats[i].IsoLine_valid[j] = true;
      } else {
        if (!left && (j < MAXISOLINES - 2)) {
          left = true;
          target = GetTurnpointTarget(i);

          j++;

          // insert start point (again)
          TaskStats[i].IsoLine_Geo[j] = target;
          TaskStats[i].IsoLine_valid[j] = true;
        }
      }

      j++;
    }
  }
}
