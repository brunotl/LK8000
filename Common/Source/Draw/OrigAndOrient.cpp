/*
   LK8000 Tactical Flight Computer -  WWW.LK8000.IT
   Released under GNU/GPL License v.2 or later
   See CREDITS.TXT file for authors and copyrights

   $Id$
*/

#include "externs.h"

bool MapWindow::GliderCenter = false;

void MapWindow::SetOrientation(double Display, double Aircraft, bool Center) {
  GliderCenter = Center;
  DisplayAngle = Display;
  DisplayAircraftAngle = Aircraft - Display;
}

void MapWindow::CalculateOrientationNormal() {
  double trackbearing = DrawInfo.TrackBearing;
  double waypointbearing = DerivedDrawInfo.WaypointBearing;

  auto Orientation = []() -> int {
    if (mode.autoNorthUP()) {
      return NORTHUP;
    }
    switch (DisplayOrientation) {
      case TARGETCIRCLE:
        return mode.Is(Mode::MODE_CIRCLING) ? TARGETUP : TRACKUP;  // Target up in circling, Track up in cruise
      case NORTHCIRCLE:
        return mode.Is(Mode::MODE_CIRCLING) ? NORTHUP : TRACKUP;  // North up in circling, Track up in cruise
      case NORTHTRACK:
        return mode.Is(Mode::MODE_CIRCLING) ? TRACKUP : NORTHUP;  // Trackup in circling, North up in cruise
      default:
        break;
    }
    return DisplayOrientation;
  };

  bool Center = mode.Is(Mode::MODE_CIRCLING);  // always center when circling

  switch (Orientation()) {
    case NORTHUP:
    case NORTHSMART:
      SetOrientation(0.0, trackbearing, Center);
      break;
    case TARGETUP:
      SetOrientation(waypointbearing, trackbearing, Center);
      break;
    default:
      // TRACKUP : Normal, glider forward
      SetOrientation(trackbearing, trackbearing, Center);
      break;
  }

  DisplayAngle = AngleLimit360(DisplayAngle);
  DisplayAircraftAngle = AngleLimit360(DisplayAircraftAngle);
}

void MapWindow::CalculateOrientationTargetPan() {
  // Target pan mode, show target up when looking at current task point,
  // otherwise north up.

  if (MapWindow::mode.autoNorthUP()) {
    // North up
    SetOrientation(0.0, DrawInfo.TrackBearing, true);
  }
  else if (ActiveTaskPoint == TargetPanIndex) {
    // Target-up
    SetOrientation(DerivedDrawInfo.WaypointBearing, DrawInfo.TrackBearing, true);
  }
  else {
    // North up
    SetOrientation(0.0, DrawInfo.TrackBearing, true);
  }
}

RasterPoint MapWindow::GetOrigCenter(const RECT& rc) {
  return { 
    (rc.left + rc.right) / 2, 
    (rc.bottom + rc.top) / 2
  };
}

RasterPoint MapWindow::GetOrigTargetPan(const RECT& rc, int targetPanSize, bool isLandscape) {
  return {
    (rc.left + rc.right - (isLandscape ? targetPanSize : 0)) / 2,
    (rc.bottom + rc.top + (isLandscape ? 0 : targetPanSize)) / 2
  };
}

RasterPoint MapWindow::GetOrigNorthSmart(const RECT& rc, double trackbearing, bool isLandscape) {
  double middleXY = isLandscape 
      ? ((rc.bottom - BottomSize) + rc.top) / 2 
      : (rc.left + rc.right) / 2;
  double spanxy = NIBLSCALE(50.);

  if (isLandscape) {
    return {
      (rc.left + rc.right) / 2,
      static_cast<PixelScalar>(middleXY + (spanxy * fastcosine(trackbearing)))
    };
  } else {
    return {
      static_cast<PixelScalar>(middleXY - (spanxy * fastsine(trackbearing))),
      ((rc.bottom - BottomSize) + rc.top) / 2
    };
  }
}

RasterPoint MapWindow::GetOrigAutoOrient(const RECT& rc, double scale, double autoOrientScale) {
  if (scale > autoOrientScale) {
    return {
      (rc.left + rc.right) / 2,
      ((rc.bottom - BottomSize) + rc.top) / 2
    };
  } else {
    return {
      ((rc.right - rc.left) * GliderScreenPositionX / 100) + rc.left,
      ((rc.top - rc.bottom) * GliderScreenPositionY / 100) + rc.bottom
    };
  }
}

RasterPoint MapWindow::CalculateOrigin(const RECT& rc) {
  if (mode.Is(Mode::MODE_TARGET_PAN)) {
    CalculateOrientationTargetPan();
    return GetOrigCenter(rc);
  }

  if (mode.Is(Mode::MODE_PAN)) {
    SetOrientation(0.0, DrawInfo.TrackBearing, true); // North up
    return GetOrigCenter(rc);
  }

  CalculateOrientationNormal();

  if (mode.Is(Mode::MODE_CIRCLING)) {
    return GetOrigCenter(rc);
  }

  if (DisplayOrientation == NORTHSMART) {
    return GetOrigNorthSmart(rc, DrawInfo.TrackBearing, ScreenLandscape);
  }

  return GetOrigAutoOrient(rc, zoom.Scale(), AutoOrientScale);
}

// change dynamically the map orientation mode
// set true flag for resetting DisplayOrientation mode and return
void MapWindow::SetAutoOrientation() {
  // 1.4 because of correction if mapscale reported on screen in MapWindow2
  MapWindow::mode.setAutoNorthUP(MapWindow::zoom.Scale() > AutoOrientScale);
}
