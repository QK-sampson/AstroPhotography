/*
 * GuiderConversions.cpp -- conversions between ice and astro
 *
 * (c) 2014 Prof Dr Andreas Mueller, Hochschule Rapperswil
 */
#include <IceConversions.h>
#include <includes.h>
#include <AstroIO.h>
#include <AstroUtils.h>
#include <AstroFormat.h>
#include <iostream>
#include <sstream>

namespace snowstar {

std::string	guiderdescriptor2name(const GuiderDescriptor& descriptor) {
	std::ostringstream	out;
	out << descriptor.instrumentname;
	out << "|";
	if (descriptor.ccdIndex >= 0) {
		out << descriptor.ccdIndex;
	}
	out << "|";
	if (descriptor.guiderportIndex >= 0) {
		out << descriptor.guiderportIndex;
	}
	out << "|";
	if (descriptor.adaptiveopticsIndex >= 0) {
		out << descriptor.adaptiveopticsIndex;
	}
	std::string	guidername = out.str();
	debug(LOG_DEBUG, DEBUG_LOG, 0, "guidername: %s", guidername.c_str());
	return guidername;
}

GuiderDescriptor	guiderdescriptorParse(const std::string& name) {
	GuiderDescriptor	descriptor;
	descriptor.ccdIndex = -1;
	descriptor.guiderportIndex = -1;
	descriptor.adaptiveopticsIndex = -1;
	std::vector<std::string>	v;
	astro::split(name, "|", v);
	if (4 != v.size()) {
		std::string	cause = astro::stringprintf(
			"'%s' has %d != 4 cmponents", name.c_str(), v.size());
		debug(LOG_ERR, DEBUG_LOG, 0, "%s", cause.c_str());
		throw std::runtime_error(cause);
	}
	descriptor.instrumentname = v[0];
	if (0 != v[1].size()) {
		try {
			descriptor.ccdIndex = std::stoi(v[1]);
		} catch (const std::exception& x) {
			debug(LOG_ERR, DEBUG_LOG, 0,
				"cannot parse ccd index %s", v[1].c_str());
		}
	}
	if (0 != v[2].size()) {
		try {
			descriptor.guiderportIndex = std::stoi(v[2]);
		} catch (const std::exception& x) {
			debug(LOG_ERR, DEBUG_LOG, 0,
				"cannot parse guiderport index %s",
				v[2].c_str());
		}
	}
	if (0 != v[3].size()) {
		try {
			descriptor.adaptiveopticsIndex = std::stoi(v[3]);
		} catch (const std::exception& x) {
			debug(LOG_ERR, DEBUG_LOG, 0,
				"cannot parse adaptiveoptics index %s",
				v[3].c_str());
		}
	}
	debug(LOG_DEBUG, DEBUG_LOG, 0,
		"parsed descriptor: %s, ccd=%d, guiderport=%d, "
		"adaptiveoptics = %d", descriptor.instrumentname.c_str(),
		descriptor.ccdIndex, descriptor.guiderportIndex,
		descriptor.adaptiveopticsIndex);
	return descriptor;
}

GuiderState	convert(const astro::guiding::Guide::state& state) {
	switch (state) {
	case astro::guiding::Guide::unconfigured:
		return GuiderUNCONFIGURED;
	case astro::guiding::Guide::idle:
		return GuiderIDLE;
	case astro::guiding::Guide::calibrating:
		return GuiderCALIBRATING;
	case astro::guiding::Guide::calibrated:
		return GuiderCALIBRATED;
	case astro::guiding::Guide::guiding:
		return GuiderGUIDING;
	}
	throw std::runtime_error("unknown guider state");
}

astro::guiding::Guide::state	convert(const GuiderState& state) {
	switch (state) {
	case GuiderUNCONFIGURED:
		return astro::guiding::Guide::unconfigured;
	case GuiderIDLE:
		return astro::guiding::Guide::idle;
	case GuiderCALIBRATING:
		return astro::guiding::Guide::calibrating;
	case GuiderCALIBRATED:
		return astro::guiding::Guide::calibrated;
	case GuiderGUIDING:
		return astro::guiding::Guide::guiding;
	}
	throw std::runtime_error("unknown guider state");
}

GuiderState	string2guiderstate(const std::string& s) {
	return convert(astro::guiding::Guide::string2state(s));
}

std::string	guiderstate2string(GuiderState state) {
	return astro::guiding::Guide::state2string(convert(state));
}

GuiderDescriptor        convert(const astro::guiding::GuiderDescriptor& gd) {
	GuiderDescriptor	result;
	result.instrumentname = gd.instrument();
	result.ccdIndex = -1;
	result.ccdIndex = instrumentName2index(gd.instrument(),
				InstrumentGuiderCCD, gd.ccd());
	result.guiderportIndex = -1;
	if (gd.guiderport().size()) {
		result.guiderportIndex = instrumentName2index(gd.instrument(),
				InstrumentGuiderPort, gd.guiderport());
	}
	result.adaptiveopticsIndex = -1;
	if (gd.adaptiveoptics().size()) {
		result.adaptiveopticsIndex
			= instrumentName2index(gd.instrument(),
				InstrumentGuiderPort, gd.adaptiveoptics());
	}
	return result;
}

/**
 * \brief Convert a guider descriptor into a locally useful guider name
 */
astro::guiding::GuiderDescriptor        convert(const GuiderDescriptor& gd) {
	// convert ccd index into a name
	std::string	ccdname = instrumentIndex2name(gd.instrumentname,
				InstrumentGuiderCCD, gd.ccdIndex);
	// convert guiderport index into a name
	std::string	guiderportname;
	if (gd.guiderportIndex >= 0) {
		guiderportname = instrumentIndex2name(gd.instrumentname,
			InstrumentGuiderPort, gd.guiderportIndex);
	}
	// convert adaptiveoptics index into a name
	std::string	adaptiveopticsname;
	if (gd.adaptiveopticsIndex >= 0) {
		adaptiveopticsname = instrumentIndex2name(
			gd.instrumentname,
			InstrumentAdaptiveOptics, gd.adaptiveopticsIndex);
	}
	// construct the guider descriptor
	astro::guiding::GuiderDescriptor	result(
			guiderdescriptor2name(gd), gd.instrumentname,
			ccdname, guiderportname, adaptiveopticsname);
	return result;
}

GuiderDescriptor        convertname(const astro::guiding::GuiderName& name) {
	GuiderDescriptor	result;
	result.instrumentname = name.instrument();
	result.ccdIndex = name.ccdIndex();
	result.guiderportIndex = name.guiderportIndex();
	result.adaptiveopticsIndex = name.adaptiveopticsIndex();
	return result;
}

astro::guiding::GuiderName      convertname(const GuiderDescriptor& name) {
	return astro::guiding::GuiderName(name.instrumentname, name.ccdIndex,
		name.guiderportIndex, name.adaptiveopticsIndex);
}

TrackingPoint   convert(const astro::guiding::TrackingPoint& trackingpoint) {
	TrackingPoint	result;
	result.timeago = converttime(trackingpoint.t);
	result.trackingoffset = convert(trackingpoint.trackingoffset);
	result.activation = convert(trackingpoint.correction);
	switch (trackingpoint.type) {
	case astro::guiding::GP:
		result.type = ControlGuiderPort;
		break;
	case astro::guiding::AO:
		result.type = ControlAdaptiveOptics;
		break;
	}
	return result;
}

astro::guiding::TrackingPoint   convert(const TrackingPoint& trackingpoint) {
	astro::guiding::TrackingPoint	result;
	result.t = converttime(trackingpoint.timeago);
	result.trackingoffset = convert(trackingpoint.trackingoffset);
	result.correction = convert(trackingpoint.activation);
	switch (trackingpoint.type) {
	case ControlGuiderPort:
		result.type = astro::guiding::GP;
		break;
	case ControlAdaptiveOptics:
		result.type = astro::guiding::AO;
		break;
	}
	return result;
}

astro::guiding::TrackingHistory	convert(const TrackingHistory& history) {
	astro::guiding::TrackingHistory	result;
	result.instrument = history.guider.instrumentname;
	result.guiderportcalid = history.guiderportcalid;
	result.adaptiveopticscalid = history.adaptiveopticscalid;
	result.ccd = instrumentIndex2name(result.instrument,
		InstrumentGuiderCCD, history.guider.ccdIndex);
	if (history.guider.guiderportIndex >= 0) {
		result.guiderport = instrumentIndex2name(result.instrument,
			InstrumentGuiderPort,
			history.guider.guiderportIndex);
	}
	if (history.guider.adaptiveopticsIndex >= 0) {
		result.adaptiveoptics = instrumentIndex2name(result.instrument,
			InstrumentAdaptiveOptics,
			history.guider.adaptiveopticsIndex);
	}
	result.whenstarted = converttime(history.timeago);
	for (auto ptr = history.points.begin(); ptr != history.points.end();
		ptr++) {
		result.points.push_back(convert(*ptr));
	}
	return result;
}

TrackingHistory	convert(const astro::guiding::TrackingHistory& history) {
	TrackingHistory	result;
	result.trackid = history.trackid;
	result.guider.instrumentname = history.instrument;
	result.guiderportcalid = history.guiderportcalid;
	result.adaptiveopticscalid = history.adaptiveopticscalid;
	result.guider.instrumentname = history.instrument;
	result.guider.ccdIndex = instrumentName2index(history.instrument,
		InstrumentGuiderCCD, history.ccd);
	result.guider.guiderportIndex = -1;
	if (history.guiderport.size()) {
		result.guider.guiderportIndex = instrumentName2index(
			history.instrument, InstrumentGuiderPort,
			history.guiderport);
	}
	result.guider.adaptiveopticsIndex = -1;
	if (history.adaptiveoptics.size()) {
		result.guider.adaptiveopticsIndex = instrumentName2index(
			history.instrument, InstrumentAdaptiveOptics,
			history.adaptiveoptics);
	}
	result.timeago = converttime(history.whenstarted);
	for (auto ptr = history.points.begin(); ptr != history.points.end();
		ptr++) {
		result.points.push_back(convert(*ptr));
	}
	return result;
}

ControlType     convertcontroltype(
	const astro::guiding::ControlDeviceType& caltype) {
	switch (caltype) {
	case astro::guiding::GP:
		return ControlGuiderPort;
	case astro::guiding::AO:
		return ControlAdaptiveOptics;
	}
}

astro::guiding::ControlDeviceType       convertcontroltype(
	const ControlType& caltype) {
	switch (caltype) {
	case ControlGuiderPort:
		return astro::guiding::GP;
	case ControlAdaptiveOptics:
		return astro::guiding::AO;
	}
}

CalibrationPoint        convert(const astro::guiding::CalibrationPoint& cp) {
	CalibrationPoint	result;
	result.t = cp.t;
	result.offset = convert(cp.offset);
	result.star = convert(cp.star);
	return result;
}

astro::guiding::CalibrationPoint        convert(const CalibrationPoint& cp) {
	astro::guiding::CalibrationPoint	result;
	result.t = cp.t;
	result.offset = convert(cp.offset);
	result.star = convert(cp.star);
	return result;
}

Calibration     convert(const astro::guiding::CalibrationPtr cal) {
	Calibration	result;
	result.id = cal->calibrationid();
	result.timeago = converttime(cal->when());
	debug(LOG_DEBUG, DEBUG_LOG, 0, "convert calibration %d time %d to %f",
		cal->calibrationid(), cal->when(), result.timeago);
	result.guider = convertname(cal->name());
	result.type = convertcontroltype(cal->calibrationtype());
	result.focallength = cal->focallength();
	result.masPerPixel = cal->masPerPixel();
	for (int i = 0; i < 6; i++) {
		result.coefficients.push_back(cal->a[i]);
	}
	result.complete = cal->complete();
	result.flipped = cal->flipped();
	result.det  = cal->det();
	result.quality = cal->quality();

	return result;
}

astro::guiding::CalibrationPtr	convert(const Calibration& cal) {
	debug(LOG_DEBUG, DEBUG_LOG, 0, "convert calibration %d time %f",
		cal.id, cal.timeago);
	astro::guiding::GuiderName	guidername = convertname(cal.guider);
	astro::guiding::ControlDeviceName	cdname(guidername,
		convertcontroltype(cal.type));
	astro::guiding::BasicCalibration	*result = NULL;
	switch (cal.type) {
	case ControlGuiderPort:
		result = new astro::guiding::GuiderCalibration(cdname);
		break;
	case ControlAdaptiveOptics:
		result = new astro::guiding::AdaptiveOpticsCalibration(cdname);
		break;
	}
	result->calibrationid(cal.id);
	result->when(converttime(cal.timeago));
	//result->calibrationtype(convertcontroltype(cal.type));

	for (int i = 0; i < 6; i++) {
		result->a[i] = cal.coefficients[i];
	}
	result->complete(cal.complete);
	result->flipped(cal.flipped);
	result->masPerPixel(cal.masPerPixel);
	result->focallength(cal.focallength);
	return astro::guiding::CalibrationPtr(result);
}

struct TrackingSummary	convert(const astro::guiding::TrackingSummary& summary) {
	struct TrackingSummary	result;
	result.guider.instrumentname = summary.descriptor.instrument();
	result.guider.ccdIndex = instrumentName2index(
		summary.descriptor.instrument(), InstrumentGuiderCCD,
		summary.descriptor.ccd());
	if (summary.descriptor.guiderport().size()) {
		result.guider.guiderportIndex = instrumentName2index(
			summary.descriptor.instrument(), InstrumentGuiderPort,
			summary.descriptor.guiderport());
	} else {
		result.guider.guiderportIndex = -1;
	}
	if (summary.descriptor.adaptiveoptics().size()) {
		result.guider.adaptiveopticsIndex = instrumentName2index(
			summary.descriptor.instrument(),
			InstrumentAdaptiveOptics,
			summary.descriptor.adaptiveoptics());
	} else {
		result.guider.adaptiveopticsIndex = -1;
	}
	result.since = converttime(summary.starttime);
	result.guiderportcalid = summary.guiderportcalid;
	result.adaptiveopticscalid = summary.adaptiveopticscalid;
	result.trackid = summary.trackingid;
	result.points = summary.count();
	result.lastoffset = convert(summary.lastoffset);
	result.averageoffset = convert(summary.averageoffset());
	result.variance = convert(summary.variance());
	return result;
}

astro::guiding::TrackingSummary	convert(const struct TrackingSummary& summary) {
	std::string	name = guiderdescriptor2name(summary.guider);
	std::string	ccdname = instrumentIndex2name(
		summary.guider.instrumentname, InstrumentGuiderCCD,
		summary.guider.ccdIndex);
	std::string	guiderportname;
	if (summary.guider.guiderportIndex >= 0) {
		guiderportname = instrumentIndex2name(
			summary.guider.instrumentname, InstrumentGuiderPort,
			summary.guider.guiderportIndex);
	}
	std::string	adaptiveopticsname;
	if (summary.guider.adaptiveopticsIndex >= 0) {
		adaptiveopticsname = instrumentIndex2name(
			summary.guider.instrumentname, InstrumentAdaptiveOptics,
			summary.guider.adaptiveopticsIndex);
	}
	astro::guiding::TrackingSummary	result(name,
		summary.guider.instrumentname,
		ccdname, guiderportname, adaptiveopticsname);
	result.starttime = converttime(summary.since);
	result.trackingid = summary.trackid;
	result.guiderportcalid = summary.guiderportcalid;
	result.adaptiveopticscalid = summary.adaptiveopticscalid;
	result.count(summary.points);
	result.lastoffset = convert(summary.lastoffset);
	result.average(convert(summary.averageoffset));
	result.variance(convert(summary.variance));
	return result;
}

std::string	calibrationtype2string(ControlType caltype) {
	switch (caltype) {
	case ControlGuiderPort:
		return std::string("GP");
	case ControlAdaptiveOptics:
		return std::string("AO");
	}
	std::string	cause = astro::stringprintf("unknown cal type %d",
				caltype);
	debug(LOG_ERR, DEBUG_LOG, 0, "%s", cause.c_str());
	throw std::runtime_error(cause);
}

ControlType	string2calibrationtype(const std::string& caltype) {
	if ((caltype == "GP") || (caltype == "GuiderPort")) {
		return ControlGuiderPort;
	}
	if ((caltype == "AO") || (caltype == "AdaptiveOptics")) {
		return ControlAdaptiveOptics;
	}
	std::string	cause = astro::stringprintf("unknown cal type '%s'",
		caltype.c_str());
	debug(LOG_ERR, DEBUG_LOG, 0, "%s", cause.c_str());
	throw std::runtime_error(cause);
}

} // namespace snowstar
