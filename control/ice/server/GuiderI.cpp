/*
 * GuiderI.cpp -- guider servern implementation
 *
 * (c) 2014 Prof Dr Andreas Mueller, Hochschule Rapperswil
 */
#include <GuiderI.h>
#include <IceConversions.h>
#include <CameraI.h>
#include <CcdI.h>
#include <GuiderPortI.h>
#include <ImagesI.h>
#include <CalibrationStore.h>
#include <AstroGuiding.h>
#include <AstroConfig.h>
#include <TrackingPersistence.h>
#include <TrackingStore.h>
#include "CalibrationSource.h"
#include <AstroEvent.h>

namespace snowstar {

/**
 * \brief calback adapter for Tracking monitor
 */
template<>
void	callback_adapter<TrackingMonitorPrx>(TrackingMonitorPrx& p,
		const astro::callback::CallbackDataPtr data) {
	// check whether the info we got is 
	astro::guiding::TrackingPoint   *trackinginfo
		= dynamic_cast<astro::guiding::TrackingPoint *>(&*data);

	// leave immediately, if there is not Tracking info
	if (NULL == trackinginfo) {
		debug(LOG_DEBUG, DEBUG_LOG, 0, "not a tracking info object");
		return;
	}

	// tracking 
	TrackingPoint	trackingpoint = convert(*trackinginfo);
	p->update(trackingpoint);
}

/**
 * \brief Function to copy image pixels to the SimpleImage structure
 */
template<typename pixel>
void	copy_image(const astro::image::Image<pixel> *source,
		SimpleImage& target, double scale) {
	for (int y = 0; y < target.size.height; y++) {
		for (int x = 0; x < target.size.width; x++) {
			unsigned short	value = scale * source->pixel(x, y);
			target.imagedata.push_back(value);
		}
	}
}

#define copypixels(pixel, scale, source, target)		\
do {								\
	astro::image::Image<pixel>	*im			\
		= dynamic_cast<astro::image::Image<pixel> *>(	\
			&*source);				\
	if (NULL != im) {					\
		copy_image(im, target, scale);			\
	}							\
} while (0)

/**
 * \brief calback adapter for Tracking monitor
 */
template<>
void	callback_adapter<ImageMonitorPrx>(ImageMonitorPrx& p,
		const astro::callback::CallbackDataPtr data) {
	debug(LOG_DEBUG, DEBUG_LOG, 0, "image callback called");
	// first check whether we really got an image
	astro::callback::ImageCallbackData	*imageptr
		= dynamic_cast<astro::callback::ImageCallbackData *>(&*data);
	if (NULL == imageptr) {
		debug(LOG_DEBUG, DEBUG_LOG, 0, "ignoring non-ImageCallbackData");
		return;
	}

	// source image
	astro::image::ImagePtr	source = imageptr->image();
	debug(LOG_DEBUG, DEBUG_LOG, 0, "callback image has size %s",
		source->size().toString().c_str());

	// convert the image so that it is understood by the
	// ImageMonitor proxy
	SimpleImage	target;
	target.size = convert(source->size());

	// copy pixels to the target structure
	copypixels(unsigned short, 1, source, target);
	copypixels(unsigned char, 256, source, target);
	copypixels(unsigned long, (1. / 65536), source, target);
	copypixels(double, 1, source, target);
	copypixels(float, 1, source, target);
	
	if ((0 == target.size.width) && (0 == target.size.height)) {
		debug(LOG_ERR, DEBUG_LOG, 0,
			"don't know how to handle non short images");
		return;
	}

	// now that the image has been created, send it to the callback
	p->update(target);
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
	astro::guiding::GuiderCalibrationCallbackData	*calibration
		= dynamic_cast<astro::guiding::GuiderCalibrationCallbackData *>(&*data);
	if (NULL != calibration) {
		p->stop();
		return;
	}
}

/**
 * \brief Constructor for the Guider servant
 */
GuiderI::GuiderI(astro::guiding::GuiderPtr _guider,
	astro::image::ImageDirectory& _imagedirectory,
	astro::persistence::Database _database)
	: guider(_guider), imagedirectory(_imagedirectory),
	  database(_database) {
	// set point to an invalid value to allow us to detect that it 
	// has not been set
	_point.x = -1;
	_point.y = -1;
	// default tracking method is star
	_method = TrackerSTAR;

	// callback stuff
	debug(LOG_DEBUG, DEBUG_LOG, 0, "installing  callbacks");

	// guider calibration callback, called for calibration points and
	// completed calibrations
	GuiderICalibrationCallback	*ccallback
		= new GuiderICalibrationCallback(*this);
	_calibrationcallback = astro::callback::CallbackPtr(ccallback);
	guider->addCalibrationCallback(_calibrationcallback);
	guider->addGuidercalibrationCallback(_calibrationcallback);

	// image callback, called for every image taken by the imager of
	// the guider
	GuiderIImageCallback	*icallback = new GuiderIImageCallback(*this);
	_imagecallback = astro::callback::CallbackPtr(icallback);
	guider->addImageCallback(_imagecallback);

	// tracking callback, called for every tracking point processed by
	// either of the control devices of the guider
	GuiderITrackingCallback	*tcallback = new GuiderITrackingCallback(*this);
	_trackingcallback = astro::callback::CallbackPtr(tcallback);
	guider->addTrackingCallback(_trackingcallback);
}

/**
 * \brief Guider destructor
 *
 * The main purpose of the destructor is to unregister the callbacks
 * that were registered during construction.
 */
GuiderI::~GuiderI() {
	guider->removeCalibrationCallback(_calibrationcallback);
	guider->removeGuidercalibrationCallback(_calibrationcallback);
	guider->removeImageCallback(_imagecallback);
	guider->removeTrackingCallback(_trackingcallback);
}

GuiderState GuiderI::getState(const Ice::Current& /* current */) {
	return convert(guider->state());
}

CcdPrx GuiderI::getCcd(const Ice::Current& current) {
	std::string	ccdname = guider->getDescriptor().ccd();
	return CcdI::createProxy(ccdname, current);
}

GuiderPortPrx GuiderI::getGuiderPort(const Ice::Current& current) {
	std::string	name = guider->getDescriptor().guiderport();
	return GuiderPortI::createProxy(name, current);
}

GuiderDescriptor GuiderI::getDescriptor(const Ice::Current& /* current */) {
	return convert(guider->getDescriptor());
}

void GuiderI::setExposure(const Exposure& exposure,
	const Ice::Current& /* current */) {
	debug(LOG_DEBUG, DEBUG_LOG, 0, "set exposure: time=%f",
		exposure.exposuretime);
	guider->exposure(convert(exposure));
}

Exposure GuiderI::getExposure(const Ice::Current& /* current */) {
	return convert(guider->exposure());
}

void GuiderI::setStar(const Point& point, const Ice::Current& /* current */) {
	_point = point;
}

Point GuiderI::getStar(const Ice::Current& /* current */) {
	return _point;
}

TrackerMethod	GuiderI::getTrackerMethod(const Ice::Current& /* current */) {
	return _method;
}

void	GuiderI::setTrackerMethod(TrackerMethod method, const Ice::Current& /* current */) {
	debug(LOG_DEBUG, DEBUG_LOG, 0, "using method: %s",
		(method == TrackerUNDEFINED) ? "undefined" : (
			(method == TrackerSTAR) ? "star" : (
				(method == TrackerPHASE) ? "phase" : "diff")));
	_method = method;
}

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
		astro::event(EVENT_CLASS, astro::events::Event::GUIDE,
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
	case ControlGuiderPort:
		guider->guiderPortDevice->flip();
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
		case ControlGuiderPort:
			astro::event(EVENT_CLASS, astro::events::Event::GUIDE,
				astro::stringprintf("GP %s uncalibrated",
				guider->name().c_str()));
			guider->unCalibrate(astro::guiding::BasicCalibration::GP);
			break;
		case ControlAdaptiveOptics:
			astro::event(EVENT_CLASS, astro::events::Event::GUIDE,
				astro::stringprintf("AO %s uncalibrated",
				guider->name().c_str()));
			guider->unCalibrate(astro::guiding::BasicCalibration::AO);
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
Calibration GuiderI::getCalibration(ControlType calibrationtype, const Ice::Current& /* current */) {
	CalibrationSource	source(database);
	Calibration	calibration;
	switch (calibrationtype) {
	case ControlGuiderPort:
		if (!guider->guiderPortDevice->iscalibrated()) {
			throw BadState("GP not calibrated");
		}
		calibration = source.get(guider->guiderPortDevice->calibrationid());
		calibration.flipped = guider->guiderPortDevice->flipped();
		return calibration;
	case ControlAdaptiveOptics:
		if (!guider->adaptiveOpticsDevice->iscalibrated()) {
			throw BadState("GP not calibrated");
		}
		calibration = source.get(guider->adaptiveOpticsDevice->calibrationid());
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
Ice::Int GuiderI::startCalibration(ControlType caltype, const Ice::Current& /* current */) {
	debug(LOG_DEBUG, DEBUG_LOG, 0, "start calibration, type = %s",
		calibrationtype2string(caltype).c_str());

	// construct a tracker
	astro::guiding::TrackerPtr	tracker = getTracker();

	// start the calibration
	switch (caltype) {
	case ControlGuiderPort:
		astro::event(EVENT_CLASS, astro::events::Event::GUIDE,
			astro::stringprintf("start GP %s calibration",
			guider->name().c_str()));
		return guider->startCalibration(astro::guiding::BasicCalibration::GP, tracker);
	case ControlAdaptiveOptics:
		astro::event(EVENT_CLASS, astro::events::Event::GUIDE,
			astro::stringprintf("start AO %s calibration",
			guider->name().c_str()));
		return guider->startCalibration(astro::guiding::BasicCalibration::AO, tracker);
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
	guider->guiderPortDevice->cancelCalibration();
}

/**
 * \brief Wait for the calibration to complete
 */
bool GuiderI::waitCalibration(Ice::Double timeout,
	const Ice::Current& /* current */) {
	return guider->guiderPortDevice->waitCalibration(timeout);
}

/**
 * \brief build a tracker
 */
astro::guiding::TrackerPtr	 GuiderI::getTracker() {
	// first we must make sure the data we have is consistent
	astro::camera::Exposure	exposure = guider->exposure();
	if ((exposure.frame().size().width() <= 0) ||
		(exposure.frame().size().height() <= 0)) {
		// get the frame from the ccd
		debug(LOG_DEBUG, DEBUG_LOG, 0, "using ccd frame");
		exposure.frame(guider->imager().ccd()->getInfo().getFrame());
		guider->exposure(exposure);
	}
	if ((_point.x < 0) || (_point.y < 0)) {
		astro::image::ImagePoint	c = exposure.frame().center();
		_point.x = c.x();
		_point.y = c.y();
		debug(LOG_DEBUG, DEBUG_LOG, 0, "using ccd center (%.1f,%.1f) as star",
			_point.x, _point.y);
	}

	switch (_method) {
	case TrackerUNDEFINED:
	case TrackerNULL:
		debug(LOG_DEBUG, DEBUG_LOG, 0, "construct a NULL tracker");
		return guider->getNullTracker();
		break;
	case TrackerSTAR:
		debug(LOG_DEBUG, DEBUG_LOG, 0, "construct a star tracker");
		return guider->getTracker(convert(_point));
		break;
	case TrackerPHASE:
		debug(LOG_DEBUG, DEBUG_LOG, 0, "construct a phase tracker");
		return guider->getPhaseTracker();
		break;
	case TrackerDIFFPHASE:
		debug(LOG_DEBUG, DEBUG_LOG, 0, "construct a diff tracker");
		return guider->getDiffPhaseTracker();
		break;
	case TrackerLAPLACE:
		debug(LOG_DEBUG, DEBUG_LOG, 0, "construct a laplace tracker");
		return guider->getLaplaceTracker();
		break;
	case TrackerLARGE:
		debug(LOG_DEBUG, DEBUG_LOG, 0, "construct a large tracker");
		return guider->getLargeTracker();
		break;
	}
	debug(LOG_ERR, DEBUG_LOG, 0, "tracking method is invalid "
		"(should not happen)");
	throw BadState("tracking method");
}

/**
 * \brief Start guiding
 */
void GuiderI::startGuiding(Ice::Float gpinterval, Ice::Float aointerval,
		bool stepping,
		const Ice::Current& /* current */) {
	debug(LOG_DEBUG, DEBUG_LOG, 0, "start guiding with interval gp=%.1f, ao=%.1f",
		gpinterval, aointerval);
	// construct a tracker
	astro::guiding::TrackerPtr	tracker = getTracker();

	// start guiding
	debug(LOG_DEBUG, DEBUG_LOG, 0, "start guiding");
	guider->startGuiding(tracker, gpinterval, aointerval, stepping);
	astro::event(EVENT_CLASS, astro::events::Event::GUIDE,
		astro::stringprintf("start guiding %s",
		guider->name().c_str()));
}

Ice::Float GuiderI::getGuidingInterval(const Ice::Current& /* current */) {
	return guider->getInterval();
}

void GuiderI::stopGuiding(const Ice::Current& /* current */) {
	guider->stopGuiding();

	// send the clients that guiding was stopped
	trackingcallbacks.stop();
	//imagecallbacks.stop();

	// remove the callback
	//guider->trackingcallback(NULL);
	astro::event(EVENT_CLASS, astro::events::Event::GUIDE,
		astro::stringprintf("stop guiding %s",
		guider->name().c_str()));
}

ImagePrx GuiderI::mostRecentImage(const Ice::Current& current) {
	// retrieve image
	astro::image::ImagePtr	image = guider->mostRecentImage();
	if (!image) {
		throw NotFound("no image available");
	}

	// store image in image directory
	std::string	filename = imagedirectory.save(image);

	// return a proxy for the image
	return getImage(filename, image->bytesPerPixel(), current);
}

TrackingPoint GuiderI::mostRecentTrackingPoint(const Ice::Current& /* current */) {
	if (astro::guiding::Guide::guiding != guider->state()) {
		throw BadState("not currently guiding");
	}

	// get info from the guider
	double	lastaction;
	astro::Point	offset;
	astro::Point	activation;
	guider->lastAction(lastaction, offset, activation);
	
	// construct a tracking point
	TrackingPoint	result;
	result.timeago = converttime(lastaction);
	result.trackingoffset = convert(offset);
	result.activation = convert(activation);
	return result;
}

TrackingHistory GuiderI::getTrackingHistory(Ice::Int id,
	const Ice::Current& /* current */) {
	debug(LOG_DEBUG, DEBUG_LOG, 0, "get tracking history %d", id);
	astro::guiding::TrackingStore	store(database);
	return convert(store.get(id));
}

TrackingHistory GuiderI::getTrackingHistoryType(Ice::Int id,
	ControlType type, const Ice::Current& /* current */) {
	debug(LOG_DEBUG, DEBUG_LOG, 0, "get tracking history %d", id);
	astro::guiding::TrackingStore	store(database);
	switch (type) {
	case ControlGuiderPort:
		return convert(store.get(id,
			astro::guiding::BasicCalibration::GP));
	case ControlAdaptiveOptics:
		return convert(store.get(id,
			astro::guiding::BasicCalibration::AO));
	}
	debug(LOG_ERR, DEBUG_LOG, 0,
		"control type is invalid (should not happen)");
	throw BadState("not a valid control type");
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
 * \brief Register a callback for images taken during the process
 */
void    GuiderI::registerImageMonitor(const Ice::Identity& imagecallback,
		const Ice::Current& current) {
	debug(LOG_DEBUG, DEBUG_LOG, 0, "register an image callback");
	imagecallbacks.registerCallback(imagecallback, current);
}

/**
 * \brief Unregister a callback for images
 */
void    GuiderI::unregisterImageMonitor(const Ice::Identity& imagecallback,
		const Ice::Current& current) {
	imagecallbacks.unregisterCallback(imagecallback, current);
}

/**
 * \brief Register a callback for monitoring the tracking
 */
void    GuiderI::registerTrackingMonitor(const Ice::Identity& trackingcallback,
		const Ice::Current& current) {
	debug(LOG_DEBUG, DEBUG_LOG, 0, "register tracking callback");
	try {
		trackingcallbacks.registerCallback(trackingcallback, current);
	} catch (const std::exception& x) {
		debug(LOG_ERR, DEBUG_LOG, 0, "cannot register tracking callback: %s %s",
			astro::demangle(typeid(x).name()).c_str(), x.what());
	} catch (...) {
		debug(LOG_ERR, DEBUG_LOG, 0,
			"cannot register tracking callback for unknown reason");
	}
}

/**
 * \brief Unregister a callback for monitoring the tracking
 */
void    GuiderI::unregisterTrackingMonitor(const Ice::Identity& trackingcallback,
		const Ice::Current& current) {
	trackingcallbacks.unregisterCallback(trackingcallback, current);
}

/**
 * \brief Handle a tracking update
 *
 * This method needs to be called by the callback installed in the guider
 */
void	GuiderI::trackingUpdate(const astro::callback::CallbackDataPtr data) {
	// tell the clients that data has changed
	trackingcallbacks(data);
}

/**
 * \brief Handle a new image from the tracking process
 */
void	GuiderI::trackingImageUpdate(const astro::callback::CallbackDataPtr data) {
	debug(LOG_DEBUG, DEBUG_LOG, 0, "trackingImageUpdate called");

	if (imagerepo) {
		debug(LOG_DEBUG, DEBUG_LOG, 0, "send image to repository %s",
			_repositoryname.c_str());
		astro::callback::ImageCallbackData	*imageptr
			= dynamic_cast<astro::callback::ImageCallbackData *>(&*data);
		if (NULL == imageptr) {
			debug(LOG_DEBUG, DEBUG_LOG, 0,
				"ignoring non-ImageCallbackData");
		} else {
			// save the image in the repository
			imagerepo->save(imageptr->image());
		}
	}

	// sending data to all registered callbacks
	imagecallbacks(data);
}

/**
 * \brief Handle an update from the calibration process
 */
void	GuiderI::calibrationUpdate(const astro::callback::CallbackDataPtr data) {

	// send calibration callbacks to the registered callbacks
	calibrationcallbacks(data);
}

/**
 * \brief Handle the summary retrieval method
 */
TrackingSummary	GuiderI::getTrackingSummary(const Ice::Current& /* current */) {
	debug(LOG_DEBUG, DEBUG_LOG, 0, "calling for tracking summary");
	if (astro::guiding::Guide::guiding != guider->state()) {
		BadState	exception;
		exception.cause = astro::stringprintf("guider is not wrong "
			"state %s", astro::guiding::Guide::state2string(
			guider->state()).c_str());
		throw exception;
	}
	return convert(guider->summary());
}

/**
 * \brief retrieve the name of the current repository
 */
std::string	GuiderI::getRepositoryName(const Ice::Current& /* current */) {
	return _repositoryname;
}

/**
 * \brief activate sending images to the repository
 */
void	GuiderI::setRepositoryName(const std::string& reponame,
		const Ice::Current& /* current */) {
	// special case: zero length repo name means turn of storing images
	// in the repository
	if (0 == reponame.size()) {
		debug(LOG_DEBUG, DEBUG_LOG, 0, "removing repository '%s'",
			_repositoryname.c_str());
		_repositoryname = reponame;
		imagerepo.reset();
		return;
	}

	// check that this repository actually exists
	astro::config::ImageRepoConfigurationPtr	config
		= astro::config::ImageRepoConfiguration::get();
	if (!config->exists(reponame)) {
		// throw an error
		NotFound	exception;
		exception.cause = astro::stringprintf("repository %s not found",
			reponame.c_str());
		debug(LOG_ERR, DEBUG_LOG, 0, "%s", exception.cause.c_str());
		throw exception;
	}
	imagerepo = config->repo(reponame);
	_repositoryname = reponame;
	debug(LOG_DEBUG, DEBUG_LOG, 0, "using repository %s",
		_repositoryname.c_str());
}

} // namespace snowstar
