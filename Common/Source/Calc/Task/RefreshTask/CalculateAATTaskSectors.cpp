/*
   LK8000 Tactical Flight Computer -  WWW.LK8000.IT
   Released under GNU/GPL License v.2 or later
   See CREDITS.TXT file for authors and copyrights

   $Id$
*/

#include "externs.h"
#include "Waypointparser.h"
#include "NavFunctions.h"
#include "CalcTask.h"

extern bool TargetDialogOpen;

namespace {

double GetAATMaxRadius(int taskwaypoint) {
  if (Task[taskwaypoint].AATType == sector_type_t::SECTOR) {
    return Task[taskwaypoint].AATSectorRadius;
  }

  return Task[taskwaypoint].AATCircleRadius;
}

bool FindAATEntryDistance(double latitude, double longitude, int taskwaypoint,
                          double bearing, double* entry_distance) {
  if (entry_distance == nullptr || !ValidTaskPointFast(taskwaypoint)) {
    return false;
  }

  if (InTurnSector({{latitude, longitude}, 0}, taskwaypoint)) {
    *entry_distance = 0.0;
    return true;
  }

  double distance_to_center = 0.0;
  DistanceBearing(latitude, longitude,
                  WayPointList[Task[taskwaypoint].Index].Latitude,
                  WayPointList[Task[taskwaypoint].Index].Longitude,
                  &distance_to_center, nullptr);

  const double max_radius = GetAATMaxRadius(taskwaypoint);
  const double step = std::max(200.0, max_radius / 10.0);
  const double search_limit = distance_to_center + (2.0 * max_radius) + 2000.0;

  double lower = 0.0;
  for (double upper = step; upper <= search_limit; upper += step) {
    double t_lat = 0.0;
    double t_lon = 0.0;
    FindLatitudeLongitude(latitude, longitude, bearing, upper, &t_lat, &t_lon);

    if (!InTurnSector({{t_lat, t_lon}, 0}, taskwaypoint)) {
      lower = upper;
      continue;
    }

    // Refine first boundary crossing between last outside and first inside sample.
    double lo = lower;
    double hi = upper;
    for (int n = 0; n < 20; ++n) {
      const double mid = (lo + hi) * 0.5;
      FindLatitudeLongitude(latitude, longitude, bearing, mid, &t_lat, &t_lon);
      if (InTurnSector({{t_lat, t_lon}, 0}, taskwaypoint)) {
        hi = mid;
      } else {
        lo = mid;
      }
    }

    *entry_distance = hi;
    return true;
  }

  return false;
}

}  // namespace

void CalculateAATTaskSectors() {
  const std::lock_guard lock(CritSec_TaskData);

  int awp = ActiveTaskPoint;

  if(!UseAATTarget())
    return;

  double latitude = GPS_INFO.Latitude;
  double longitude = GPS_INFO.Longitude;
  double altitude = GPS_INFO.Altitude;

  Task[0].AATTargetOffsetRadius = 0.0;
  Task[0].AATTargetOffsetRadial = 0.0;
  if (Task[0].Index >= 0) {
    Task[0].AATTargetLat = WayPointList[Task[0].Index].Latitude;
    Task[0].AATTargetLon = WayPointList[Task[0].Index].Longitude;
  }

  for(int i=1;i<MAXTASKPOINTS;i++) {
    if(ValidTaskPointFast(i)) {
      if (!ValidTaskPointFast(i+1)) {
        // This must be the final waypoint, so it's not an AAT OZ
        Task[i].AATTargetLat = WayPointList[Task[i].Index].Latitude;
        Task[i].AATTargetLon = WayPointList[Task[i].Index].Longitude;
        continue;
      }

      // JMWAAT: if locked, don't move it
      if (i < awp) {
        // only update targets for current/later waypoints
        continue;
      }

      Task[i].AATTargetOffsetRadius = std::clamp(Task[i].AATTargetOffsetRadius, -1.0, 1.0);
      Task[i].AATTargetOffsetRadial = std::clamp(Task[i].AATTargetOffsetRadial, -90.0, 90.0);

      double targetrange;

      double targetbearing = AngleLimit360(Task[i].Bisector+Task[i].AATTargetOffsetRadial);

      if(Task[i].AATType == sector_type_t::SECTOR) {
        targetrange = ((Task[i].AATTargetOffsetRadius + 1.0) / 2.0);

        double aatbisector = HalfAngle(Task[i].AATStartRadial, Task[i].AATFinishRadial);

        if (fabs(AngleLimit180(aatbisector - targetbearing)) > 90) {
          // bisector is going away from sector
          targetbearing = Reciprocal(targetbearing);
          targetrange = 1.0 - targetrange;
        }
        if (!AngleInRange(Task[i].AATStartRadial, Task[i].AATFinishRadial, targetbearing, true)) {
          // Bisector is not within AAT sector, so
          // choose the closest radial as the target line

          if (fabs(AngleLimit180(Task[i].AATStartRadial - targetbearing)) <
              fabs(AngleLimit180(Task[i].AATFinishRadial - targetbearing))) {
            targetbearing = Task[i].AATStartRadial;
          } else {
            targetbearing = Task[i].AATFinishRadial;
          }
        }

        targetrange *= Task[i].AATSectorRadius;
      } else {
        targetrange = Task[i].AATTargetOffsetRadius * Task[i].AATCircleRadius;
      }

      // For the active waypoint, use aircraft-relative projection so the target
      // remains continuous before and after sector entry.

      bool target_updated = false;

      if ((awp == i) && !Task[i].AATTargetLocked) {
        const bool in_sector = InTurnSector({{latitude, longitude}, altitude}, i);

        double qdist;
        double bearing;
        DistanceBearing(Task[i-1].AATTargetLat,
                        Task[i-1].AATTargetLon,
                        latitude,
                        longitude,
                        &qdist, &bearing);
        bearing = AngleLimit360(bearing + Task[i].AATTargetOffsetRadial);

        double dist = -1.0;
        if (in_sector) {
          dist = ((Task[i].AATTargetOffsetRadius + 1.0) / 2.0) *
                 FindInsideAATSectorDistance(latitude, longitude, i, bearing);
        } else {
          double entry_distance = 0.0;
          if (FindAATEntryDistance(latitude, longitude, i, bearing, &entry_distance)) {
            double exit_distance =
              FindInsideAATSectorDistance(latitude, longitude, i, bearing, entry_distance);

            if (DoOptimizeRoute()) {
              // Outside the sector, optimized route should lead to the entry edge,
              // not to a center-like point that causes a dogleg.
              dist = entry_distance;
            } else {
              const double range = std::clamp((Task[i].AATTargetOffsetRadius + 1.0) / 2.0,
                                              0.0, 1.0);
              dist = entry_distance + ((exit_distance - entry_distance) * range);
            }
          }
        }

        if (dist >= 0.0) {
          double candidate_lat = 0.0;
          double candidate_lon = 0.0;
          FindLatitudeLongitude(latitude, longitude, bearing, dist,
                               &candidate_lat, &candidate_lon);

          // Keep continuity if candidate becomes much worse on total path geometry.
          const double current_score =
            DoubleLegDistance(i, Task[i].AATTargetLon, Task[i].AATTargetLat);
          const double candidate_score = DoubleLegDistance(i, candidate_lon, candidate_lat);

          if (candidate_score + 30.0 >= current_score) {
            Task[i].AATTargetLat = candidate_lat;
            Task[i].AATTargetLon = candidate_lon;
            UpdateTargetAltitude(Task[i]);
            TargetModified = true;
            target_updated = true;
          }
        }
      }

      if (!target_updated) {
        FindLatitudeLongitude (WayPointList[Task[i].Index].Latitude,
                               WayPointList[Task[i].Index].Longitude,
                               targetbearing,
                               targetrange,
                               &Task[i].AATTargetLat,
                               &Task[i].AATTargetLon);

        UpdateTargetAltitude(Task[i]);
        TargetModified = true;
      }
    }
  }

  CalculateAATIsoLines();
  if (!TargetDialogOpen) {
    TargetModified = false;
    // allow target dialog to detect externally changed targets
  }
}
