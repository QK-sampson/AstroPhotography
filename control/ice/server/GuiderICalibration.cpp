/*
 * GuiderI.cpp -- guider servern implementation
 *
 * (c) 2014 Prof Dr Andreas Mueller, Hochschule Rapperswil
 */
#include <GuiderI.h>
#include <IceConversions.h>
#include <CameraI.h>
#include <CcdI.h>
#include <GuidePortI.h>
#include <ImagesI.h>
#include <AstroGuiding.h>
#include <AstroConfig.h>
#include "CalibrationSource.h"
#include <AstroEvent.h>
#include <ImageDirectory.h>

namespace snowstar {

/**
 * \brief Use a calibration
 *
 * This method directs the guider to use a specific calibration from the 
 * database. The flipped argument allows to use the calibration if it was
 * computed on the other side of the meridian.
 */
void GuiderI::useCalibration(Ice::Int calid, bool /* flipped */,
	const Ice::Current& /* current */) {
	if (calid <= 0) {
		throw BadParameter("not a valid calibration id");
	}
	// retrieve guider data from the database
	try {
		guider->useCalibration(calid);
		astro::event(EVENT_CLASS, astro::events::INFO,
			astro::events::Event::GUIDE,
			astro::stringprintf("%s now uses calibration %d",
			guider->name().c_str(), calid));
	} catch (const astro::guiding::BadState x) {
		throw BadState(x.what());
	} catch (const astro::guiding::NotFound x) {
		throw NotFound(x.what());
	}
}

/**
 * \brief Merian flip requires that we need to flip the calibration too
 */
void GuiderI::flipCalibration(ControlType type,
	const Ice::Current& /* current */) {
	switch (type) {
	case ControlGuidePort:
		guider->guidePortDevice->flip();
		break;
	case ControlAdaptiveOptics:
		guider->adaptiveOpticsDevice->flip();
		break;
	}
	throw std::runtime_error("flipCalibratoin not implemented");
}

/**
 * \brief Uncalibrate a device
 *
 * Since all configured devices are used for guiding, there must be a method
 * to uncalibrate a device so that it is no longer used for guiding.
 */
void	GuiderI::unCalibrate(ControlType calibrationtype,
	const Ice::Current& /* current */) {
	// retrieve guider data from the database
	try {
		switch (calibrationtype) {
		case ControlGuidePort:
			astro::event(EVENT_CLASS, astro::events::INFO,
				astro::events::Event::GUIDE,
				astro::stringprintf("GP %s uncalibrated",
				guider->name().c_str()));
			guider->unCalibrate(astro::guiding::GP);
			break;
		case ControlAdaptiveOptics:
			astro::event(EVENT_CLASS, astro::events::INFO,
				astro::events::Event::GUIDE,
				astro::stringprintf("AO %s uncalibrated",
				guider->name().c_str()));
			guider->unCalibrate(astro::guiding::AO);
			break;
		}
	} catch (const astro::guiding::BadState& exception) {
		throw BadState(exception.what());
	}
}

/**
 * \brief Retrieve the calibration of a device
 *
 * This method retrieves the configuration of a device. If the device is
 * unconfigured, it throws the BadState exception.
 */
Calibration GuiderI::getCalibration(ControlType calibrationtype,
		const Ice::Current& /* current */) {
	Calibration	calibration;
	switch (calibrationtype) {
	case ControlGuidePort:
		{
		if (!guider->hasGuideport()) {
			throw BadState("no guider port present");
		}
		if (!guider->guidePortDevice->iscalibrated()) {
			throw BadState("GP not calibrated");
		}
		//debug(LOG_DEBUG, DEBUG_LOG, 0, "device has cal id %d",
		//	guider->guidePortDevice->calibrationid());
		astro::guiding::CalibrationPtr	cal
			= guider->guidePortDevice->calibration();
		calibration = convert(cal);
		calibration.flipped = guider->guidePortDevice->flipped();
		return calibration;
		}
	case ControlAdaptiveOptics:
		if (!guider->hasAdaptiveoptics()) {
			throw BadState("no adaptive optics present");
		}
		if (!guider->adaptiveOpticsDevice->iscalibrated()) {
			throw BadState("GP not calibrated");
		}
		//debug(LOG_DEBUG, DEBUG_LOG, 0, "device has cal id %d",
		//	guider->adaptiveOpticsDevice->calibrationid());
		astro::guiding::CalibrationPtr	cal
			= guider->adaptiveOpticsDevice->calibration();
		calibration = convert(cal);
		calibration.flipped = guider->adaptiveOpticsDevice->flipped();
		return calibration;
	}
	debug(LOG_ERR, DEBUG_LOG, 0,
		"control type is invalid (should not happen)");
	throw BadState("not a valid control type");
}

/**
 * \brief Start a calibration for a given focal length
 *
 * The focal length is the only piece of information that we can not
 * get from anywhere else, so it has to be specified
 */
Ice::Int GuiderI::startCalibration(ControlType caltype,
		const Ice::Current& /* current */) {
	debug(LOG_DEBUG, DEBUG_LOG, 0, "start calibration, type = %s",
		calibrationtype2string(caltype).c_str());

	// construct a tracker
	astro::guiding::TrackerPtr	tracker = getTracker();

	// start the calibration
	switch (caltype) {
	case ControlGuidePort:
		astro::event(EVENT_CLASS, astro::events::INFO,
			astro::events::Event::GUIDE,
			astro::stringprintf("start GP %s calibration",
			guider->name().c_str()));
		return guider->startCalibration(astro::guiding::GP, tracker);
	case ControlAdaptiveOptics:
		astro::event(EVENT_CLASS, astro::events::INFO,
			astro::events::Event::GUIDE,
			astro::stringprintf("start AO %s calibration",
			guider->name().c_str()));
		return guider->startCalibration(astro::guiding::AO, tracker);
	}
	debug(LOG_ERR, DEBUG_LOG, 0,
		"control type is invalid (should not happen)");
	throw BadState("not a valid control type");
}

/**
 * \brief Retrieve the current progress figure of the calibration
 */
Ice::Double GuiderI::calibrationProgress(const Ice::Current& /* current */) {
	return guider->calibrationProgress();
}

/**
 * \brief cancel the current calibration process
 */
void GuiderI::cancelCalibration(const Ice::Current& /* current */) {
	guider->guidePortDevice->cancelCalibration();
}

/**
 * \brief Wait for the calibration to complete
 */
bool GuiderI::waitCalibration(Ice::Double timeout,
	const Ice::Current& /* current */) {
	return guider->guidePortDevice->waitCalibration(timeout);
}

/**
 * \brief Register a callback for the calibration process
 */
void	GuiderI::registerCalibrationMonitor(const Ice::Identity& calibrationcallback, const Ice::Current& current) {
	debug(LOG_DEBUG, DEBUG_LOG, 0, "calibration callback registered");
	try {
		calibrationcallbacks.registerCallback(calibrationcallback,
			current);
	} catch (const std::exception& x) {
		debug(LOG_ERR, DEBUG_LOG, 0, "cannot register calibration callback: %s %s",
			astro::demangle(typeid(x).name()).c_str(), x.what());
	} catch (...) {
		debug(LOG_ERR, DEBUG_LOG, 0,
			"cannot register calibration callback for unknown reason");
	}
}

/**
 * \brief Unregister a callback for the calibration process
 */
void	GuiderI::unregisterCalibrationMonitor(const Ice::Identity& calibrationcallback, const Ice::Current& current) {
	debug(LOG_DEBUG, DEBUG_LOG, 0, "calibration callback unregistered");
	calibrationcallbacks.unregisterCallback(calibrationcallback, current);
}

/**
 * \brief Handle an update from the calibration process
 */
void	GuiderI::calibrationUpdate(const astro::callback::CallbackDataPtr data) {

	// send calibration callbacks to the registered callbacks
	calibrationcallbacks(data);
}

/**
 * \brief calback adapter for Calibration monitor
 */
template<>
void	callback_adapter<CalibrationMonitorPrx>(CalibrationMonitorPrx& p,
		const astro::callback::CallbackDataPtr data) {
	// handle a calibration point callback call
	astro::guiding::CalibrationPointCallbackData	*calibrationpoint
		= dynamic_cast<astro::guiding::CalibrationPointCallbackData *>(&*data);
	if (NULL != calibrationpoint) {
		// convert the calibration point into
		CalibrationPoint	point
			= convert(calibrationpoint->data());
		p->update(point);
		return;
	}

	// handle a completed calibration callback call, by sending the stop
	// signal
	astro::guiding::CalibrationCallbackData	*calibration
		= dynamic_cast<astro::guiding::CalibrationCallbackData *>(&*data);
	if (NULL != calibration) {
		p->stop();
		return;
	}
}

} // namespace snowstar
