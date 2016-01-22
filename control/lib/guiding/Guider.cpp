/*
 * Guider.cpp -- classes implementing guiding
 *
 * (c) 2013 Prof Dr Andreas Mueller, Hochschule Rapperswil
 */
#include <includes.h>

#include <AstroGuiding.h>
#include <AstroIO.h>
#include <GuiderProcess.h>
#include <CalibrationProcess.h>
#include <AstroCallback.h>
#include <AstroUtils.h>
#include <CalibrationPersistence.h>
#include <CalibrationStore.h>

using namespace astro::image;
using namespace astro::camera;
using namespace astro::callback;
using namespace astro::persistence;

namespace astro {
namespace guiding {

/**
 * \brief Auxiliary class to ensure calibrations found are sent to the guider
 */
class GuiderCalibrationRedirector : public Callback {
	Guider	*_guider;
public:
	GuiderCalibrationRedirector(Guider *guider) : _guider(guider) { }
	CallbackDataPtr	operator()(CallbackDataPtr data);
};


CallbackDataPtr	GuiderCalibrationRedirector::operator()(CallbackDataPtr data) {
	// handle the calibration
	{
		GuiderCalibrationCallbackData	*cal
			= dynamic_cast<GuiderCalibrationCallbackData *>(&*data);
		if (NULL != cal) {
			debug(LOG_DEBUG, DEBUG_LOG, 0, "calibration update");
			_guider->saveCalibration(cal->data());
		}
	}

	// handle progress updates
	{
		ProgressInfoCallbackData	*cal
			= dynamic_cast<ProgressInfoCallbackData *>(&*data);
		if (NULL != cal) {
			debug(LOG_DEBUG, DEBUG_LOG, 0, "progress update");
			_guider->calibrationProgress(cal->data().progress);
		}
	}

	return data;
}

//////////////////////////////////////////////////////////////////////
// Guider implementation
//////////////////////////////////////////////////////////////////////

/**
 * \brief Construct a guider from 
 *
 * Since the guider includes an exposure, it also initializes the exposure
 * to some default values. The default exposure time is 1 and the
 * default frame is the entire CCD area.
 */
Guider::Guider(const std::string& instrument,
	CcdPtr ccd, GuiderPortPtr guiderport,
	Database database)
	: GuiderBase(instrument, ccd, database), _guiderport(guiderport) {
	// default exposure settings
	exposure().exposuretime(1.);
	exposure().frame(ccd->getInfo().getFrame());

	// default focallength
	_focallength = 1;

	// calibration id
	_calibrationid = 0;

	// We have to install a callback for calibrations
	CallbackPtr	calcallback(new GuiderCalibrationRedirector(this));
	addGuidercalibrationCallback(calcallback);
	addProgressCallback(calcallback);

	// create control devices
	if (guiderport) {
		guiderPortDevice = ControlDevicePtr(
			new ControlDevice<GuiderPort, GuiderCalibration>(this,
				guiderport, database));
		debug(LOG_DEBUG, DEBUG_LOG, 0, "guider port control device");
	}
	// XXX must also create an adaptive optics device

	// at this point the guider is sufficiently configured, although
	// this configuration is not sufficient for guiding
	_state.configure();
}

/**
 * \brief update progress value
 */
void	Guider::calibrationProgress(double p) {
	debug(LOG_DEBUG, DEBUG_LOG, 0, "PROGRESS %f", p);
	_progress = p;
}

/**
 * \brief Retrieve the state 
 *
 * Get the guider state. The guider keeps state information in the guider
 * state machine, so we have to convert that to the Guider::state constants.
 * This is done by the cast operator of the GuiderStateMachine class.
 */
Guide::state	Guider::state() const {
	Guide::state	result = _state;
	return result;
}

/**
 * \brief Set a calibration
 *
 * If the calibration data is already known, then we can immediately set
 * the calibration without going through the calibration process each time
 * we build the guider.
 */
void	Guider::calibration(const GuiderCalibration& calibration) {
	_state.addCalibration();
	_calibration = calibration;
}

/**
 * \brief Cleanup for calibration processes
 *
 * If nobody waits for a calibration process, e.g. when the calibration
 * is running in a remote process, we still may want to start a new
 * calibration if the previous calibration is complete. This method
 * is intended to cleanup an old calibration process if it has already
 * terminated.
 */
void	Guider::calibrationCleanup() {
	// if we are already calibrating, we should not cleanup
	if (state() == Guide::calibrating) {
		return;
	}

	// This will implicitely cleanup the calibration process,
	// if there is one. If there is none, this operation will
	// do nothing
	calibrationprocess = NULL;
}

/**
 * \brief start an asynchronous calibration process
 *
 * This method first checks that no other calibration thread is running,
 * and if so, starts a new thread.
 */
int	Guider::startCalibration(BasicCalibration::CalibrationType type,
		TrackerPtr tracker) {
	debug(LOG_DEBUG, DEBUG_LOG, 0, "start calibration");
	// make sure we have a tracker
	if (!tracker) {
		debug(LOG_ERR, DEBUG_LOG, 0, "tracker not defined");
		throw std::runtime_error("tracker not set");
	}

	// are we in the correct state
	if (!_state.canStartCalibrating()) {
		throw std::runtime_error("wrong state");
	}
	_progress = 0;

	if ((type == BasicCalibration::GP) && guiderPortDevice) {
		debug(LOG_DEBUG, DEBUG_LOG, 0, "start GuiderPort calibration");
		_state.startCalibrating();
		guiderPortDevice->parameter("focallength", focallength());
		return guiderPortDevice->startCalibration(tracker);
	}
	if ((type == BasicCalibration::AO) && adaptiveOpticsDevice) {
		debug(LOG_DEBUG, DEBUG_LOG, 0, "start AO calibration");
		_state.startCalibrating();
		return adaptiveOpticsDevice->startCalibration(tracker);
	}
	debug(LOG_ERR, DEBUG_LOG, 0, "cannot calibrate");
	throw std::runtime_error("bad state");
}

/**
 * \brief save a guider calibration
 */
void	Guider::saveCalibration(const GuiderCalibration& cal) {
	debug(LOG_DEBUG, DEBUG_LOG, 0, "saving completed calibration %d",
		cal.calibrationid());
	_state.addCalibration();
}

/**
 * \brief cancel a calibration that is still in progress
 */
void	Guider::cancelCalibration() {
	if (_state != Guide::calibrating) {
		throw std::runtime_error("not currently calibrating");
	}
	calibrationprocess->stop();
}

/**
 * \brief wait for the calibration to complete
 */
bool	Guider::waitCalibration(double timeout) {
	if (_state != Guide::calibrating) {
		throw std::runtime_error("not currently calibrating");
	}
	return calibrationprocess->wait(timeout);
}

/**
 * \brief get a good measure for the pixel size of the CCD
 *
 * This method returns the average of the pixel dimensions, this will
 * give strange values for binned cameras. Binning looks like a strange
 * idea for a guide camera anyway.
 */
double	Guider::getPixelsize() {
	astro::camera::CcdInfo  info = ccd()->getInfo();
	float	_pixelsize = (info.pixelwidth() * exposure().mode().x()
			+ info.pixelheight() * exposure().mode().y()) / 2.;
	debug(LOG_DEBUG, DEBUG_LOG, 0, "pixelsize: %.2fum",
		1000000 * _pixelsize);
	return _pixelsize;
}

/**
 * \brief get a default tracker
 *
 * This is not the only possible tracker to use with the guiding process,
 * but it works currently quite well
 */
TrackerPtr	Guider::getTracker(const Point& point) {
	astro::camera::Exposure exp = exposure();
	debug(LOG_DEBUG, DEBUG_LOG, 0, "origin: %s",
		exp.origin().toString().c_str());
	debug(LOG_DEBUG, DEBUG_LOG, 0, "point: %s",
		point.toString().c_str());
	astro::Point    difference = point - exp.origin();
	int	x = difference.x();
	int	y = difference.y();
	astro::image::ImagePoint        trackerstar(x, y);
	astro::image::ImageRectangle    trackerrectangle(exp.size());
	astro::guiding::TrackerPtr      tracker(
		new astro::guiding::StarTracker(trackerstar,
			trackerrectangle, 10));
	return tracker;
}

TrackerPtr	Guider::getPhaseTracker() {
	debug(LOG_DEBUG, DEBUG_LOG, 0, "get a standard phase tracker");
	return TrackerPtr(new PhaseTracker());
}

TrackerPtr	Guider::getDiffPhaseTracker() {
	debug(LOG_DEBUG, DEBUG_LOG, 0, "get a differential phase tracker");
	return TrackerPtr(new DifferentialPhaseTracker());
}

/**
 * \brief start tracking
 */
void	Guider::startGuiding(TrackerPtr tracker, double interval) {
	// create a GuiderProcess instance
	_state.startGuiding();
	debug(LOG_DEBUG, DEBUG_LOG, 0, "creating new guider process");
	guiderprocess = GuiderProcessPtr(new GuiderProcess(this, guiderport(),
		tracker, interval, database()));
	debug(LOG_DEBUG, DEBUG_LOG, 0, "new guider process created");
	guiderprocess->start(tracker);
}

/**
 * \brief stop the guiding process
 */
void	Guider::stopGuiding() {
	guiderprocess->stop();
	_state.stopGuiding();
}

/**
 * \brief wait for the guiding process to terminate
 */
bool	Guider::waitGuiding(double timeout) {
	return guiderprocess->wait(timeout);
}

/**
 * \brief retrieve the interval from the guider process
 */
double	Guider::getInterval() {
	return guiderprocess->interval();
}

/**
 * \brief retrieve the tracking summary from the 
 */
const TrackingSummary&	Guider::summary() {
	if (!guiderprocess) {
		std::string	cause = stringprintf("wrong state for summary: "
			"%s", Guide::state2string(_state).c_str());
		debug(LOG_ERR, DEBUG_LOG, 0, "%s", cause.c_str());
		throw BadState(cause);
	}
	return guiderprocess->summary();
}

/**
 * \brief check the current state
 *
 * This method should always be called before the state is check. It checks
 * whether there is a current calibration or guiding process present, and
 * whether it is still running. If it has terminated, the state is updated
 * to reflect the state.
 */
void	Guider::checkstate() {
	Guide::state	s = _state;
	switch (s) {
	case Guide::unconfigured:
		break;
	case Guide::idle:
		break;
	case Guide::calibrating:
		if (calibrationprocess) {
			if (!calibrationprocess->isrunning()) {
				if (iscalibrated()) {
					_state.addCalibration();
				} else {
					_state.configure();
				}
				calibrationprocess = NULL;
			}
		}
		break;
	case Guide::calibrated:
		break;
	case Guide::guiding:
		if (guiderprocess) {
			if (!guiderprocess->isrunning()) {
				_state.addCalibration();
				guiderprocess = NULL;
			}
		}
		break;
	}
}

/**
 * \brief Retrieve information about last activation
 */
void Guider::lastAction(double& actiontime, Point& offset,
		Point& activation) {
	if (!guiderprocess) {
		throw std::runtime_error("not currently guiding");
	}
	guiderprocess->lastAction(actiontime, offset, activation);
}

/**
 * \brief Retrieve a descriptor
 */
GuiderDescriptor	Guider::getDescriptor() const {
	return GuiderDescriptor(name(), instrument(), ccdname(),
		guiderportname(), adaptiveopticsname());
}

} // namespace guiding
} // namespace astro
