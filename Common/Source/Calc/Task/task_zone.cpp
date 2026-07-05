/*
 * LK8000 Tactical Flight Computer -  WWW.LK8000.IT
 * Released under GNU/GPL License v.2 or later
 * See CREDITS.TXT file for authors and copyrights
 *
 * File:   task_sector.cpp
 * Author: Bruno de Lacheisserie
 *
 * Created on November 23, 2023
 */

#include "task_zone.h"
#include "CalcTask.h"

namespace {

template <typename T>
concept has_max_radius = requires(const T& data) {
  {data.max_radius}->std::convertible_to<double>;
};

template <typename T>
concept has_radius = requires(const T& data) {
  {data.radius}->std::convertible_to<double>;
};

struct max_radius_visitor {
  using result_type = double;

  template <typename T>
  double operator()(const T& data) const {
    if constexpr (has_max_radius<T>) {
      return data.max_radius;
    }
    else if constexpr (has_radius<T>) {
      return data.radius;
    }
    else if constexpr (std::is_same_v<T, task::dae_data>) {
      return 10000.0;  // default case for unknown types
    }
    else {
      return 0.0;
    }
  }

  double operator()(const task::dae_data&) const {
    return 10000.0;  // invalid task point, no data available
  }
};

}  // namespace

sector_type_t task::get_zone_type(int tp_index) {
  if (tp_index == 0) {
    // start ...
    return StartLine;
  }

  if (!ValidTaskPointFast(tp_index + 1)) {
    // finish
    return FinishLine;
  }

  if (UseAATTarget()) { 
    // GP or AAT task 
    return Task[tp_index].AATType;
  }
  return SectorType;
}

bool task::in_turn_sector(const GeoPoint& position,
                          const task::zone_data_variant& zone_data) {
  return std::visit(
      [&](const auto& data) {
        return InTurnSector(position, data);
      },
      zone_data);
}

double task::get_max_radius(const zone_data_variant& zone_data) {
  return std::visit(max_radius_visitor(), zone_data);
}
