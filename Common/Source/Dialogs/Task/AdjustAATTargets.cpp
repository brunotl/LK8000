/*
   LK8000 Tactical Flight Computer -  WWW.LK8000.IT
   Released under GNU/GPL License v.2 or later
   See CREDITS.TXT file for authors and copyrights

   $Id$
*/

#include "externs.h"
#include "Logger.h"

double GetAATTargetsRange() {
  const std::lock_guard lock(CritSec_TaskData);

  int istart = std::max(1, ActiveTaskPoint);
  if (!ValidTaskPointFast(istart)) {
    return 0;
  }

  double av = 0;
  int inum = 0;

  // note : no need to check ValidTaskPointFast(i) here, until "istart" is valid
  // and CritSec_TaskData is locked, because we are not changing the number of
  // task points, just their AATTargetOffsetRadius values. and no other thread
  // can change the number of task points while we are in this function.
  for (int i = istart; ValidTaskPointFast(i + 1); i++) {

    assert(Task[i].AATTargetOffsetRadius >= -1.0 &&
           Task[i].AATTargetOffsetRadius <= 1.0);

    av += Task[i].AATTargetOffsetRadius;
    inum++;
  }
  if (inum > 0) {
    av /= inum;
  }
  return av;
}

void AdjustAATTargetsRange(double desired) {
  const std::lock_guard lock(CritSec_TaskData);

  int istart = std::max(1, ActiveTaskPoint);
  if (!ValidTaskPointFast(istart)) {
    return;
  }

  // TODO accuracy: Check here for true minimum distance between
  // successive points (especially second last to final point)

  // Do this with intersection tests

  desired = std::clamp(desired, -1.0, 1.0);

  for (int i = istart; ValidTaskPointFast(i + 1); i++) {
    auto& task_point = Task[i];
    if (!task_point.AATTargetLocked) {
      task_point.AATTargetOffsetRadius = desired;
    }
  }
}
