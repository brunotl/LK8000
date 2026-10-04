/*
 * LK8000 Tactical Flight Computer -  WWW.LK8000.IT
 * Released under GNU/GPL License v.2 or later
 * See CREDITS.TXT file for authors and copyrights
 *
 * File:   Quadratic.cpp
 * Author: Bruno de Lacheisserie
 */
#include "options.h"
#include "Quadratic.h"

namespace GliderPolar {

Quadratic::Quadratic(const Point& p1, const Point& p2, const Point& p3) noexcept {
  // Solve the system of equations to find a, b, c
  // p1.y = a * p1.x^2 + b * p1.x + c
  // p2.y = a * p2.x^2 + b * p2.x + c
  // p3.y = a * p3.x^2 + b * p3.x + c

  double v1 = p1.x, w1 = p1.y;
  double v2 = p2.x, w2 = p2.y;
  double v3 = p3.x, w3 = p3.y;

  // Calculate the determinant for the system of equations.
  double d = v1 * v1 * (v2 - v3) + v2 * v2 * (v3 - v1) + v3 * v3 * (v1 - v2);
  if (d == 0.0) {
    a = b = c = 0.0;
    return;
  }

  // Calculate the coefficients for the quadratic interpolation.
  a = ((v2 - v3) * (w1 - w3) + (v3 - v1) * (w2 - w3)) / d;
  b = (w2 - w3 - a * (v2 * v2 - v3 * v3)) / (v2 - v3);
  c = w3 - a * v3 * v3 - b * v3;
}

bool Quadratic::IsValid() const {
  if (a >= 0.) {
    return false;  // "a" must be negative for a valid quadratic polar
  }
  const double minSpeed = MinSpeed();
  if (minSpeed < 0.) {
    return false;
  }
  const double sink = Sink(minSpeed);
  if (sink > 0.) {
    return false;
  }
  return true;
}

}  // namespace GliderPolar

