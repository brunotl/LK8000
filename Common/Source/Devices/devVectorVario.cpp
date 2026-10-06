/*
 * LK8000 Tactical Flight Computer -  WWW.LK8000.IT
 * Released under GNU/GPL License v.2 or later
 * See CREDITS.TXT file for authors and copyrights
 *
 * File:   devVectorVario.cpp
 * Author: Bruno de Lacheisserie
 */
#include "externs.h"
#include "devVectorVario.h"
#include <string_view>
#include <charconv>
#include <optional>
#include <vector>
#include "Comm/device.h"
#include "MessageLog.h"
#include "devGeneric.h"
#include "Comm/Bluetooth/gatt_utils.h"
#include "Comm/Bluetooth/characteristic_value.h"
#include "Calc/Vario.h"
#include "Units.h"
#include "GliderPolar/Polar.h"
#include "McReady.h"
#include "utils/strcpy.h"
#include "utils/charset_helper.h"

extern Mutex CritSec_FlightData;

namespace {

void Vario(DeviceDescriptor_t& d, NMEA_INFO& info,
           const std::vector<uint8_t>& data) {
  auto value = Units::From(unDecimeterPersecond,
                           characteristic_value<int32_t>(data).get());

  const std::lock_guard lock(CritSec_FlightData);
  UpdateVarioSource(info, d, value);
}

void VarioNetto(DeviceDescriptor_t& d, NMEA_INFO& info,
                const std::vector<uint8_t>& data) {
  auto value = Units::From(unDecimeterPersecond,
                           characteristic_value<int32_t>(data).get());

  const std::lock_guard lock(CritSec_FlightData);
  info.NettoVario.update(d, value);
}

void TAS(DeviceDescriptor_t& d, NMEA_INFO& info,
         const std::vector<uint8_t>& data) {
  auto value = Units::From(unDecimeterPersecond,
                           characteristic_value<int32_t>(data).get());

  const std::lock_guard lock(CritSec_FlightData);
  info.TrueAirSpeed.update(d, value);
}

void IAS(DeviceDescriptor_t& d, NMEA_INFO& info,
         const std::vector<uint8_t>& data) {
  auto value = Units::From(unDecimeterPersecond,
                           characteristic_value<int32_t>(data).get());

  const std::lock_guard lock(CritSec_FlightData);
  info.IndicatedAirSpeed.update(d, value);
}

void Azimuth(DeviceDescriptor_t& d, NMEA_INFO& info,
         const std::vector<uint8_t>& data) {
  auto value = characteristic_value<uint16_t>(data).get();

  const std::lock_guard lock(CritSec_FlightData);
  info.MagneticHeading.update(d, value);
}

void GLoad(DeviceDescriptor_t& d, NMEA_INFO& info,
            const std::vector<uint8_t>& data) {
  auto value = characteristic_value<int32_t>(data).get();
  const std::lock_guard lock(CritSec_FlightData);
  info.Gload.update(d, value / 10.0);
}

void Roll(DeviceDescriptor_t& d, NMEA_INFO& info,
            const std::vector<uint8_t>& data) {
  double value = characteristic_value<int16_t>(data).get()  / 10.;
  const std::lock_guard lock(CritSec_FlightData);
  GyroscopeData GyroData = info.Gyroscope.value(); 
  GyroData.Roll = value;
  info.Gyroscope.update(d, std::move(GyroData));
}

void Pitch(DeviceDescriptor_t& d, NMEA_INFO& info,
            const std::vector<uint8_t>& data) {
  double value = characteristic_value<int16_t>(data).get() / 10.;
  const std::lock_guard lock(CritSec_FlightData);
  GyroscopeData GyroData = info.Gyroscope.value(); 
  GyroData.Pitch = value;
  info.Gyroscope.update(d, std::move(GyroData));
}

#ifndef NDEBUG
void VarioMode(DeviceDescriptor_t& d, NMEA_INFO& info,
           const std::vector<uint8_t>& data) {
  auto value = characteristic_value<uint8_t>(data).get();
  DebugLog(_T("VarioMode : %d"), value);
  // TODO : 0 = Classic, 1 = Total energy
}
#endif

std::vector<std::string_view> ParseNmea(const std::string_view& sv) {
  std::vector<std::string_view> fields;
  fields.reserve(32);
  size_t start = 0;
  while (start <= sv.size()) {
    size_t pos = sv.find(',', start);
    if (pos == std::string_view::npos) {
      fields.emplace_back(sv.substr(start));
      break;
    }
    fields.emplace_back(sv.substr(start, pos - start));
    start = pos + 1;
  }
  return fields;
}

enum XCTODFields : unsigned {
  FW_VERSION = 7,
  TAKEOFF,
  SD_CARD,
  SOUND_LEVEL,
  PILOT_NAME,
  PILOT_ID,
  CONFIG,
  SOUND_GND,
  SOUND_BLE,
  OPTION_RR,
  NETTO_PLUS,
  NETTO_MINUS,
  NETTO_GLIDE,
  CALCULATION_MODE,
  INTEGRATION_TIME,
  WING_CHOICE,
  GLIDER_MODEL,
  GLIDER_ID,
  PROJECTED_SURFACE,
  FLAT_ASPECT_RATIO,
  HARNESS,
  AUW
};

std::optional<double> ToDouble(std::string_view sv) {
  double value;
  auto res = std::from_chars(sv.data(), sv.data() + sv.size(), value);
  if (res.ec != std::errc{}) {
    return std::nullopt;
  }
  return value;
}

void Xctod(DeviceDescriptor_t& d, NMEA_INFO& info,
          const std::vector<uint8_t>& data) {
  try {
    std::string_view sv(reinterpret_cast<const char*>(data.data()),
                        data.size());
    if (sv.starts_with("$XCTOD,")) {
      std::vector<std::string_view> fields = ParseNmea(sv);
      if (fields.size() <= AUW) {
        return;  // Not enough fields
      }
      auto ps = ToDouble(fields[PROJECTED_SURFACE]);
      auto ar = ToDouble(fields[FLAT_ASPECT_RATIO]);
      auto harness = fields[HARNESS];
      auto auw = ToDouble(fields[AUW]);

      if (!ps || !ar || !auw || *ps <= 0 || *ar <= 0 || *auw <= 0) {
        return;  // Invalid data
      }
      if (GliderPolar::Update(*ps, *ar, *auw, harness)) {
        from_unknown_charset(std::string(fields[GLIDER_MODEL]).c_str(), szPolarName);
        GlidePolar::SetBallast();
      }
    }
  }
  catch (const std::exception& e) {
    DebugLog(_T("VectorVario Test: %s"), to_tstring(e.what()).c_str());
  }
}

using OnGattCharacteristicT = std::function<void(
    DeviceDescriptor_t&, NMEA_INFO&, const std::vector<uint8_t>&)>;

struct DataHandlerT {
  OnGattCharacteristicT OnGattCharacteristic;
};

using service_table_t = bluetooth::service_table_t<DataHandlerT>;

const service_table_t& service_table() {
  using bluetooth::gatt_uuid;
  static const service_table_t table = {
      // BLE SERVICE Vector Vario
      {{"2fce4890-0197-47e0-a825-d4777b9a5d67",
        {{
            {"2fce4891-0197-47e0-a825-d4777b9a5d67", {&Vario}},
            {"2fce4892-0197-47e0-a825-d4777b9a5d67", {&TAS}},
            {"2fce4893-0197-47e0-a825-d4777b9a5d67", {&IAS}},
            {"2fce4895-0197-47e0-a825-d4777b9a5d67", {&GLoad}},
            {"2fce4896-0197-47e0-a825-d4777b9a5d67", {&Azimuth}},
            {"2fce4902-0197-47e0-a825-d4777b9a5d67", {&VarioNetto}},
            {"2fce4897-0197-47e0-a825-d4777b9a5d67", {&Roll}},
            {"2fce4898-0197-47e0-a825-d4777b9a5d67", {&Pitch}},
            {"2fce4899-0197-47e0-a825-d4777b9a5d67", {&Xctod}},
#ifndef NDEBUG
            {"2fce4903-0197-47e0-a825-d4777b9a5d67", {&VarioMode}},
#endif
        }}}}};
  return table;
}

bool DoEnableGattCharacteristic(DeviceDescriptor_t& d, uuid_t service,
                                uuid_t characteristic) {
  return service_table().get(service, characteristic);
}

void OnGattCharacteristic(DeviceDescriptor_t& d, NMEA_INFO& info,
                          uuid_t service, uuid_t characteristic,
                          const std::vector<uint8_t>& data) {
  auto handler = service_table().get(service, characteristic);
  if (handler) {
    std::invoke(handler->OnGattCharacteristic, d, info, data);
  }
}

}  // namespace

void VectorVario::Install(DeviceDescriptor_t* d) {
  genInstall(d);  // install Generic driver callback first

  d->DoEnableGattCharacteristic = DoEnableGattCharacteristic;
  d->OnGattCharacteristic = OnGattCharacteristic;
}
