/*
 * LK8000 Tactical Flight Computer -  WWW.LK8000.IT
 * Released under GNU/GPL License v.2 or later
 * See CREDITS.TXT file for authors and copyrights
 *
 * $Id: LDRotaryBuffer.h,v 1.1 2011/12/21 10:28:45 root Exp root $
 */
#include <list>
#include <optional>

struct NMEA_INFO;
struct DERIVED_INFO;

class LDRotary final {
 public:
  LDRotary() = default;

  void Init();
  void Insert(double distance, const NMEA_INFO *Basic, DERIVED_INFO *Calculated);
  void Calculate(const NMEA_INFO *Basic, DERIVED_INFO *Calculated);

 private:
  static double GetPeriod();

  struct DataT {
    double distance = 0.;
    double altitude = 0.;
    double ias = 0.;
    double speed = 0.;

    DataT& operator+=(DataT data) {
      distance += data.distance;
      altitude += data.altitude;
      ias += data.ias;
      speed += data.speed;
      return *this;
    }

    DataT& operator-=(DataT data) {
      distance -= data.distance;
      altitude -= data.altitude;
      ias -= data.ias;
      speed -= data.speed;
      return *this;
    }
  };

  struct TimedDataT {
    double time;
    DataT data;
  };

private:
  std::list<TimedDataT> buffer;
  DataT total = {};

  std::optional<double> last_altitude = 0.;
  double period = GetPeriod();
};

extern LDRotary rotaryLD;
