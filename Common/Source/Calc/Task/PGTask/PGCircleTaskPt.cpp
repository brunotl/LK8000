/*
 * LK8000 Tactical Flight Computer -  WWW.LK8000.IT
 * Released under GNU/GPL License v.2 or later
 * See CREDITS.TXT file for authors and copyrights
 * 
 * File:   PGCircleTaskPt.cpp
 * Author: Bruno de Lacheisserie
 * 
 * Created on 6 octobre 2012, 12:27
 */

#include "PGTaskPt.h"
#include "PGCircleTaskPt.h"
#include "Library/newuoa.h"

namespace {

class OptimizedDistance final {
public:

    OptimizedDistance(const ProjPt& prev, const ProjPt& cur, const ProjPt& next, const double radius) 
            : m_prev(prev), m_cur(cur), m_next(next), m_radius(radius) { }

    double operator()(int n, double* theta) const {
      ProjPt optPoint = {
        m_cur.x + m_radius * cos(*theta),
        m_cur.y + m_radius * sin(*theta)
      };

      return Distance(m_prev, optPoint) + Distance(optPoint, m_next);
    }

private:
    const ProjPt& m_prev;
    const ProjPt& m_cur;
    const ProjPt& m_next;
    const double m_radius;
};

} // namespace

PGCircleTaskPt::PGCircleTaskPt(ProjPt&& point, double Radius)
    : PGTaskPt(std::forward<ProjPt>(point)), m_Radius(Radius) { }

void PGCircleTaskPt::Optimize(const ProjPt& prev, const ProjPt& next) {
    if(m_Radius == 0.0){
        return;
    }

    if (IsNull(m_Optimized)) {
        // first run : init m_Optimized with center ...
        m_Optimized = m_Center;
    }
    const auto& a = prev;
    const auto& b = IsNull(next) ? m_Center : next;

    if (!CrossPoint(a, b, m_Optimized)) {
        // If the line segment from prev to next does not cross the circle, we
        // need to optimize the point on the circle that minimizes the distance to
        // prev and next.

        OptimizedDistance Fmin(a, m_Center, b, m_Radius);

        double best_x = 0;
        double best_distance = std::numeric_limits<double>::max();

        // to avoid local minima, we choose a starting point for the optimization
        // using a simple grid search over 16 points around the circle
        for (int i = 0; i < 16; ++i) {
            double x0 = (M_PI / 8.0) * i;
            double distance = Fmin(1, &x0);
            if (distance < best_distance) {
                best_distance = distance;
                best_x = x0;
            }
        }

        // Use the best starting point for the final optimization
        const double rb = M_PI / 4.0;
        best_distance = min_newuoa(1, &best_x, Fmin, rb, 0.01 / m_Radius);

        m_Optimized = {
            m_Center.x + m_Radius * cos(best_x), 
            m_Center.y + m_Radius * sin(best_x)
        };
    }
}

bool PGCircleTaskPt::CrossPoint(const ProjPt& prev, const ProjPt& next, ProjPt& optimized) {
    ProjPt A = prev - m_Center;
    ProjPt B = next - m_Center;
    if (A == B) {
        // Next and prev is same point -> ignore next...
        B = {0, 0};
    }

    ProjPt AB = B - A;
    ProjPt::scalar_type a = (AB.x * AB.x) + (AB.y * AB.y);
    if (a == 0) {
        return false;  // no cross point
    }

    ProjPt A2(A.x * A.x, A.y * A.y);
    ProjPt::scalar_type R2 = (m_Radius * m_Radius);

    ProjPt::scalar_type b = 2 * ((AB.x * A.x) + (AB.y * A.y));
    ProjPt::scalar_type c = A2.x + A2.y - R2;

    double bb4ac = (b * b) - (4 * a * c);
    if (bb4ac < 0) {
        // no cross point
        return false;
    }

    auto valid = [](double k) {
        return k >= 0.0 && k <= 1.0;
    };

    double k = 0;
    if (bb4ac == 0) {
        // one point
        k = -b / (2 * a);
        if (!valid(k)) {
            return false; // tangent point outside segment
        }
    }
    else if (bb4ac > 0) {
        // Two point
        double s = sqrt(bb4ac);

        double k1 = (-b - s) / (2 * a);
        double k2 = (-b + s) / (2 * a);

        bool valid1 = valid(k1);
        bool valid2 = valid(k2);

        if (!valid1 && !valid2) {
            return false; // both points outside segment
        }

        if (valid1 && valid2) {
            // both points inside segment, choose the one closer to prev
            k = std::min(k1, k2);
        }
        else if (valid1) {
            // only k1 is valid, prev point is outside, next point is inside
            k = k1;
        }
        else {
            // only k2 is valid, prev point is inside, next point is outside
            k = k2;
        }
    }

    optimized = prev + ((next - prev) * k);
    return true;
}
