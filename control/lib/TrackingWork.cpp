/*
 * TrackingWork.cpp -- Tracking Process
 *
 * (c) 2013 Prof Dr Andreas Mueller, Hochschule Rapperswil
 */
#include <TrackingWork.h>
#include <AstroDebug.h>
#include <AstroUtils.h>
#include <sstream>
#include <iomanip>

namespace astro {
namespace guiding {

/**
 * \brief output a tracking history entry
 */
std::ostream&	operator<<(std::ostream& out, const trackinghistoryentry& entry) {
	out << std::fixed << std::setprecision(3) << entry.first;
	out << ",";
	out << entry.second;
	return out;
}

/**
 * \brief convert a tracking history entry to string
 */
std::string	toString(const trackinghistoryentry& entry) {
	std::ostringstream	out;
	out << entry;
	return out.str();
}

/**
 * \brief Construct a new tracking process
 *
 * The tracking process uses the offsets measured by the tracker and
 * the calibration information from the guider to compute corrections.
 * The initialization ensures that the normal drift of the mount is
 * corrected even when no tracking is active. 
 */
TrackingWork::TrackingWork(Guider& _guider, TrackerPtr _tracker,
	DrivingWork& driving)
	: GuidingProcess(_guider, _tracker), _driving(driving) {
	// set a default gain
	_gain = 1;
	_interval = 10;

	// set the default tracking length -1, which turns the history off
	_history_length = -1;

	// compute the ra/dec duty cycle to compensate the drift
	// (the vx, vy speed found in the calibration). We determine these
	// using the 
	const GuiderCalibration&	calibration = guider().calibration();

	// the default correction only neutralizes the drift
	Point	correction = calibration.defaultcorrection();
	double	tx = -correction.x();
	double	ty = -correction.y();
	debug(LOG_DEBUG, DEBUG_LOG, 0, "tx = %.3fs, ty = %.3fs", tx, ty);
	if ((fabs(tx) > 1) || (fabs(ty) > 1)) {
		std::string	msg = stringprintf("default activation times "
			"%.3f, %.3f out of range", tx, ty);
		debug(LOG_ERR, DEBUG_LOG, 0, "%s", msg.c_str());
		throw std::runtime_error(msg);
	}

	// inform the drive thread about the default correction. This will
	// correct the default offset for 1 second.
	// We expect to have done something more useful by then
	_driving.setCorrection(tx, ty);
}

/**
 * \brief Destruct the thread
 *
 * Stop the thread and wait for it's termination. This is needed as the
 * driving process may need data structures that would otherwise go away
 * before it terminates. The wait time is at most _interval + 1.
 */
TrackingWork::~TrackingWork() {
	try {
		stop();
		wait(_interval + 1);
	} catch (std::exception& x) {
		debug(LOG_DEBUG, DEBUG_LOG, 0,
			"TrackingWork destructor throws exception: %s",
			x.what());
	}
}

/**
 * \brief Set the interval
 *
 * This accessor enforces the minimum time interval of 1 second, allthough
 * it is not quite clear why we want this, we certainly could go quicker.
 */
void	TrackingWork::interval(const double& i) {
	// remember the tracker
	if (i < 1) {
		std::string	msg = stringprintf("cannot guide in %.3f "
			"second intervals: minimum 1", i);
		debug(LOG_DEBUG, DEBUG_LOG, 0, "%s", msg.c_str());
		throw std::runtime_error(msg);
	}
	_interval = i;
}

/**
 * \brief Main function for the tracking
 */
void	TrackingWork::main(GuidingThread<TrackingWork>& thread) {
	debug(LOG_DEBUG, DEBUG_LOG, 0, "TRACK: tracker main function started");
	// every time we go through the loop we ask whether we should terminate
	// we also do this at appropriate points within the loop
	while (!thread.terminate()) {
		// we measure the time it takes to get an exposure. This
		// may be larger than the interval, so we need the time
		// to protect from overcorrecting
		Timer	timer;
		timer.start();

		debug(LOG_DEBUG, DEBUG_LOG, 0, "TRACK: start new exposure");
		// initiate an exposure
		guider().startExposure();

		// until the image is exposed
		Timer::sleep(guider().exposure().exposuretime);
		debug(LOG_DEBUG, DEBUG_LOG, 0, "TRACK: exposure complete");

		// now retrieve the image
		ImagePtr	image = guider().getImage();
		timer.end();
		debug(LOG_DEBUG, DEBUG_LOG, 0,
			"TRACK: new image received, elapsed = %f",
			timer.elapsed());

		// use the tracker to find the tracking offset
		Point	offset = tracker()->operator()(image);
		debug(LOG_DEBUG, DEBUG_LOG, 0,
			"TRACK: current tracker offset: %s",
			offset.toString().c_str());

		// find out whether the tracker can still track, terminate
		// if not
		if ((offset.x() != offset.x()) || (offset.y() != offset.y())) {
			debug(LOG_ERR, DEBUG_LOG, 0, "loss of tracking");
			return;
		}

		// The correction should happen within a certain time.
		// Ideally, when we come back with the next tracker image,
		// the offset should have been corrected completely.
		// So the time it takes to correct should either be about
		// the time it takes to take a picture, but at least the
		// interval value. This means that if it takes longer to
		// to take a picture than the interval warrants, then
		// we make corrections according to 
		double	correctiontime = timer.elapsed();
		if (correctiontime < interval()) {
			correctiontime = interval();
		}
		debug(LOG_DEBUG, DEBUG_LOG, 0,
			"TRACK: using correction interval %f", 
			correctiontime);

		// compute the correction to tx and ty
		Point	correction = gain() * guider().calibration()(offset,
				correctiontime);
		debug(LOG_DEBUG, DEBUG_LOG, 0,
			"TRACK: offset = %s, correction = %s",
			offset.toString().c_str(),
			correction.toString().c_str());

		// add the correction to the history
		addHistory(correction);

		// compute the correction, but this must be done with tx, ty
		// protected from concurrent access, because it may be in an
		// illegal state temporarily. We also have to divide by the
		// interval, assuming that we will correct the offset by the
		// time we get the next image
		double	tx = -correction.x();
		double	ty = -correction.y();

		// inform the drive thread about what it should do next
		_driving.setCorrection(tx, ty);
		//_driving.setCorrection(0, 0);

		// remember information for monitoring
		_lastaction = Timer::gettime();
		_offset = offset;
		_activation = -correction;

		// this is a possible cancellation point
		if (thread.terminate()) {
			return;
		}

		// now ensure that we don't correct more often than specified
		// by the interval
		double	sleeptime = interval() - timer.elapsed();
		if(sleeptime > 0) {
			debug(LOG_DEBUG, DEBUG_LOG, 0,
				"TRACK: sleep for %f seconds",
				sleeptime);
			Timer::sleep(sleeptime);
		}
	}
}

/**
 * \brief Add a new entry to the tracking history
 *
 * This method removes entries from the front of the tracking history if
 * the history would otherwise become too large.
 * \param point		new tracking offset
 */
void	TrackingWork::addHistory(const Point& point) {
	// return immediately if the tracking history is turned off
	if (_history_length < 0) {
		return;
	}
	double	now = Timer::gettime();
	trackinghistoryentry	entry(now, point);
	debug(LOG_DEBUG, DEBUG_LOG, 0, "new tracking history entry: %s",
		toString(entry).c_str());
	// _history_length == 0 means unlimited tracking history length
	if (_history_length > 0) {
		while (trackinghistory.size() > _history_length) {
			trackinghistory.pop_front();
		}
	}
	trackinghistory.push_back(entry);
	// for debugging
	dumpHistory(std::cout);
}

/**
 * \brief Dump the tracking history to a stream
 */
void	TrackingWork::dumpHistory(std::ostream& out) {
	trackinghistory_type::const_iterator	i;
	for (i = trackinghistory.begin(); i != trackinghistory.end(); i++) {
		out << (*i) << std::endl;
	}
}

/**
 * \brief retrieve last action information
 */
void	TrackingWork::lastAction(double& actiontime, Point& offset,
		Point& activation) {
	actiontime = Timer::gettime() - _lastaction;
	offset = _offset;
	activation = _activation;
}

} // namespace guiding
} // namespace astro