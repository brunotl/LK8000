/*
   LK8000 Tactical Flight Computer -  WWW.LK8000.IT
   Released under GNU/GPL License v.2 or later
   See CREDITS.TXT file for authors and copyrights

   $Id: LDRotaryBuffer.cpp,v 1.1 2011/12/21 10:28:45 root Exp root $
*/

#include "externs.h"
#include "McReady.h"
#include "LDRotaryBuffer.h"
#include "utils/unique_file_ptr.h"
#include "utils/lookup_table.h"

LDRotary rotaryLD;

double LDRotary::GetPeriod() {
	constexpr auto table = lookup_table<AverEffTime_t, double>({
		{ ae3seconds, 3 },
		{ ae5seconds, 5 },
		{ ae10seconds, 10 },
		{ ae15seconds, 15 },
		{ ae30seconds, 30 },
		{ ae45seconds, 45 },
		{ ae60seconds, 60 },
		{ ae90seconds, 90 },
		{ ae2minutes, 120 },
		{ ae3minutes, 180 }
	});
	return table.get(static_cast<AverEffTime_t>(AverEffTime), 3);
}

void LDRotary::Init() {
	buffer.clear();
	last_altitude.reset();
	total = {};
	period = GetPeriod();
}

void LDRotary::Insert(double distance, const NMEA_INFO *Basic, DERIVED_INFO *Calculated) {
	if (Calculated->OnGround) {
		return;
	}
	if (!ISCAR) {
		if (Calculated->Circling) {
			DebugLog(_T("Circling, ignore LDrotary"));
			return;
		}
	}

	if (last_altitude) {
		DataT new_data = {
			distance,
			(*last_altitude) - Calculated->NavAltitude,
			Basic->IndicatedAirspeed,
			Basic->Speed
		};

		total += new_data;

		buffer.push_back({ Basic->Time, std::move(new_data) });
	}
	last_altitude = Calculated->NavAltitude;

	while (!buffer.empty()) {
		const auto& old_data = buffer.front();
		if ((old_data.time + period) <= Basic->Time) {
			total -= old_data.data;
			buffer.pop_front();
		} else {
			return;
		}
	}
}

void LDRotary::Calculate(const NMEA_INFO *Basic, DERIVED_INFO *Calculated) {
	if ( buffer.empty() || Calculated->Circling || Calculated->OnGround || !Calculated->Flying ) {
		Calculated->EqMc = -1;
		Calculated->AverageLD = 0;
		Calculated->AverageGS = 0;
		Calculated->AverageDistance = 0;
		return;
	}

	double delta_time = buffer.back().time - buffer.front().time;
	if (delta_time <= 0.1) {
		return;
	}

	double averias = total.ias / delta_time;
	double avertas = TrueAirSpeed(averias, QNHAltitudeToQNEAltitude(Basic->Altitude));
	if (avertas > (GlidePolar::Vminsink() * 0.6)) {
		Calculated->EqMc = GlidePolar::EquMC(averias);
		// Do not consider impossible MC values as Equivalent
		if (Calculated->EqMc > 20) {
			Calculated->EqMc = -1;
		}
	}

	Calculated->AverageLD = (total.altitude > 0.)
								? total.distance / total.altitude
								: INVALID_GR;

	if (Calculated->AverageLD > MAXEFFICIENCYSHOW) {
		Calculated->AverageLD = INVALID_GR;
	}

	Calculated->AverageGS = total.speed / delta_time;
	Calculated->AverageDistance = total.distance / delta_time;
}
