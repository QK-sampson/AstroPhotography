/*
 * ControlDevice.cpp -- device 
 *
 * (c) 2016 Prof Dr Andreas Mueller, Hochschule Rapperswil
 */
#include <AstroGuiding.h>
#include <BasicProcess.h>
#include <CalibrationStore.h>
#include <CalibrationProcess.h>
#include <AOCalibrationProcess.h>
#include <algorithm>

using namespace astro::callback;

namespace astro {
namespace guiding {

/**
 * \brief Callback classes for control devices
 */
class ControlDeviceCallback : public Callback {
	ControlDeviceBase	*_controldevice;
public:
	ControlDeviceCallback(ControlDeviceBase *controldevice)
		: _controldevice(controldevice) {
	}
	CallbackDataPtr	operator()(CallbackDataPtr data);
};

/**
 * \brief processing method to process callback 
 */
CallbackDataPtr	ControlDeviceCallback::operator()(CallbackDataPtr data) {
	debug(LOG_DEBUG, DEBUG_LOG, 0,
		"control device callback called");
	// handle calibration point upates
	{
		CalibrationPointCallbackData	*cal
			= dynamic_cast<CalibrationPointCallbackData *>(&*data);
		if (NULL != cal) {
			debug(LOG_DEBUG, DEBUG_LOG, 0, "calibration point: %s",
				cal->data().toString().c_str());
			_controldevice->addCalibrationPoint(cal->data());
			return data;
		}
	}
	// handle the calibration when it completes
	{
		GuiderCalibrationCallbackData   *cal
			= dynamic_cast<GuiderCalibrationCallbackData *>(&*data);
		if (NULL != cal) {
			debug(LOG_DEBUG, DEBUG_LOG, 0, "calibration update");
			_controldevice->saveCalibration(cal->data());
			return data;
		}
	}
	// handle progress information
	{
		ProgressInfoCallbackData	*cal
			= dynamic_cast<ProgressInfoCallbackData *>(&*data);
		if (NULL != cal) {
			debug(LOG_DEBUG, DEBUG_LOG, 0, "progress update");
			if (cal->data().aborted) {
				_controldevice->calibrating(false);
			}
			return data;
		}
	}

	return data;
}

/**
 * \brief Create a new control device
 */
ControlDeviceBase::ControlDeviceBase(GuiderBase *guider,
		persistence::Database database)
	: _guider(guider), _database(database) {
	_calibrating = false;
	ControlDeviceCallback	*cb = new ControlDeviceCallback(this);
	_callback = CallbackPtr(cb);
	_guider->addGuidercalibrationCallback(_callback);
	_guider->addCalibrationCallback(_callback);
}

/**
 * \brief destroy the control device
 */
ControlDeviceBase::~ControlDeviceBase() {
	_guider->removeGuidercalibrationCallback(_callback);
	_guider->removeCalibrationCallback(_callback);

	delete _calibration;
	_calibration = NULL;
}

/**
 * \brief Retrieve the calibration
 */
void	ControlDeviceBase::calibrationid(int calid) {
	debug(LOG_DEBUG, DEBUG_LOG, 0, "set calibration: %d", calid);

	// handle special case: calid < 0 indicates that we want to remove
	// the calibration
	if (calid <= 0) {
		debug(LOG_DEBUG, DEBUG_LOG, 0, "uncalibrating %s",
			deviceType().name());
		_calibration->reset();
		return;
	}

	// we need a calibration from the store
	CalibrationStore	store(_database);
	
	// get the type of the calibration
	std::type_index	type = typeid(*_calibration);
	debug(LOG_DEBUG, DEBUG_LOG, 0, "calibration type: %s", type.name());

	// check for guider calibration
	if (type == typeid(GuiderCalibration)) {
		debug(LOG_DEBUG, DEBUG_LOG, 0, "GP calibration %d", calid);
		if (!store.containscomplete(calid, BasicCalibration::GP)) {
			throw std::runtime_error("no such calibration id");
		}
		GuiderCalibration	*gcal
			= dynamic_cast<GuiderCalibration *>(_calibration);
		if (NULL == gcal) {
			return;
		}
		*gcal = store.getGuiderCalibration(calid);
		gcal->calibrationid(calid);
		return;
	}

	// check for adaptive optics calibration
	if (type == typeid(AdaptiveOpticsCalibration)) {
		debug(LOG_DEBUG, DEBUG_LOG, 0, "AO calibration %d", calid);
		if (!store.containscomplete(calid, BasicCalibration::AO)) {
			throw std::runtime_error("no such calibration id");
		}
		AdaptiveOpticsCalibration	*acal
			= dynamic_cast<AdaptiveOpticsCalibration *>(_calibration);
		if (NULL == acal) {
			return;
		}
		*acal = store.getAdaptiveOpticsCalibration(calid);
		acal->calibrationid(calid);
		return;
	}
}

bool	ControlDeviceBase::iscalibrated() const {
	return (calibrationid() > 0) ? true : false;
}

bool	ControlDeviceBase::flipped() const {
	if (_calibration) {
		return _calibration->flipped();
	}
	return false;
}

void	ControlDeviceBase::flip() {
	if (_calibration) {
		return _calibration->flip();
	}
}

void	ControlDeviceBase::addCalibrationPoint(const CalibrationPoint& point) {
	debug(LOG_DEBUG, DEBUG_LOG, 0, "ADD CALIBRATION POINT %d %s",
		_calibration->calibrationid(), point.toString().c_str());
	if (!_calibrating) {
		debug(LOG_DEBUG, DEBUG_LOG, 0, "not calibrating!");
		return;
	}
	_calibration->add(point);
	CalibrationStore	store(_database);
	store.addPoint(_calibration->calibrationid(), point);
}

const std::string&	ControlDeviceBase::name() const {
	return _guider->name();
}

const std::string&	ControlDeviceBase::instrument() const {
	return _guider->instrument();
}

camera::Imager&	ControlDeviceBase::imager() {
	return _guider->imager();
}

std::string	ControlDeviceBase::ccdname() const {
	return _guider->ccdname();
}

const camera::Exposure&	ControlDeviceBase::exposure() const {
	return _guider->exposure();
}

void	ControlDeviceBase::exposure(const camera::Exposure& e) {
	_guider->exposure(e);
}

/**
 * \brief start the calibration
 */
int	ControlDeviceBase::startCalibration(TrackerPtr /* tracker */) {
	debug(LOG_DEBUG, DEBUG_LOG, 0, "common calibration start");
	if (_database) {
		// initialize the calibration as far as we can
		_calibration->calibrationid(0);
		if (configurationType() == typeid(GuiderCalibration)) {
			_calibration->calibrationtype(BasicCalibration::GP);
		}
		if (configurationType() == typeid(AdaptiveOpticsCalibration)) {
			_calibration->calibrationtype(BasicCalibration::AO);
		}
		CalibrationRecord	record(0, *_calibration);

		// set data describing the device
		record.name = _guider->name();
		record.instrument = _guider->instrument();
		record.ccd = _guider->ccdname();
		record.controldevice = devicename();
		debug(LOG_DEBUG, DEBUG_LOG, 0, "quality: %f", record.quality);

		// add specific attributes
		GuiderCalibration	*gcal
			= dynamic_cast<GuiderCalibration *>(_calibration);
		if (NULL != gcal) {
			record.focallength = gcal->focallength;
			record.masPerPixel = gcal->masPerPixel;
		}

		// record der Tabelle zufügen
		CalibrationTable	calibrationtable(_database);
		_calibration->calibrationid(calibrationtable.add(record));
		debug(LOG_DEBUG, DEBUG_LOG, 0,
			"saved %s calibration record id = %d",
			BasicCalibration::type2string(
				_calibration->calibrationtype()).c_str(),
			_calibration->calibrationid());
	}

	// start the process
	debug(LOG_DEBUG, DEBUG_LOG, 0, "starting process");
	process->start();
	_calibrating = true;

	debug(LOG_DEBUG, DEBUG_LOG, 0, "calibration %d started", 
		_calibration->calibrationid());
	return _calibration->calibrationid();
}

/**
 * \brief cancel the calibration process
 */
void	ControlDeviceBase::cancelCalibration() {
	process->stop();
}

/**
 * \brief wait for the calibration to complete
 */
bool	ControlDeviceBase::waitCalibration(double timeout) {
	return process->wait(timeout);
}

/**
 * \brief save a guider calibration
 */
void	ControlDeviceBase::saveCalibration(const BasicCalibration& cal) {
	debug(LOG_DEBUG, DEBUG_LOG, 0,
		"received calibration %s to save as %d, %d points",
		cal.toString().c_str(), _calibration->calibrationid(),
		cal.size());
	_calibrating = false;
	if (!_database) {
		return;
	}
	// update the calibration in the database
	BasicCalibration	calcopy = cal;
	calcopy.calibrationid(_calibration->calibrationid());
	calcopy.complete(true);
	if (calcopy.calibrationid() <= 0) {
		return;
	}
	CalibrationStore	calstore(_database);
	calstore.updateCalibration(calcopy);
	*_calibration = calcopy;
}

/**
 * \brief Check whether a parameter exists
 */
bool	ControlDeviceBase::hasParameter(const std::string& name) const {
	std::map<std::string, double>::const_iterator	i
		= parameters.find(name);
	return (i != parameters.end());
}

/**
 * \brief return value associated with a parameter
 */
double	ControlDeviceBase::parameter(const std::string& name) const {
	if (!hasParameter(name)) {
		std::string	cause = stringprintf("no value for '%s'",
			name.c_str());
		debug(LOG_ERR, DEBUG_LOG, 0, "%s", cause.c_str());
		throw std::runtime_error(cause);
	}
	return parameters.find(name)->second;
}

/**
 * \brief return value associated with a parameter, with default if not present
 */
double	ControlDeviceBase::parameter(const std::string& name, double value) const {
	if (!hasParameter(name)) {
		return value;
	}
	return parameters.find(name)->second;
}

/**
 * \brief set a parameter value
 */
void	ControlDeviceBase::setParameter(const std::string& name, double value) {
	if (hasParameter(name)) {
		parameters[name] = value;
		return;
	}
	parameters.insert(std::make_pair(name, value));
}

/**
 * \brief Computing the correction for the base: no correction
 */
Point	ControlDeviceBase::correct(const Point& point, double /* Deltat */,
		bool /* stepping */) {
	return point;
}

//////////////////////////////////////////////////////////////////////
// asynchronouos action for the guiderport
//////////////////////////////////////////////////////////////////////

/**
 * \brief action class for asynchronous guider port actions
 */
class GuiderPortAction : public Action {
	GuiderPortPtr	_guiderport;
	Point	_correction;
	double	_deltat;
	bool	_sequential;
public:
	bool	sequential() const { return _sequential; }
	void	sequential(bool s) { _sequential = s; }
private:
	bool	_stepping;
public:
	bool	stepping() const { return _stepping; }
	void	stepping(bool s) { _stepping = s; }
	
	GuiderPortAction(GuiderPortPtr guiderport, const Point& correction,
		double deltat)
		: _guiderport(guiderport), _correction(correction),
		  _deltat(deltat) {
		debug(LOG_DEBUG, DEBUG_LOG, 0, "GuiderPortAction %s",
			correction.toString().c_str());
		_sequential = false;
		_stepping = false;
	}
	void	execute();
};

/**
 * \brief execute the guiderport action
 */
void	GuiderPortAction::execute() {
	debug(LOG_DEBUG, DEBUG_LOG, 0, "guider port action started %s",
		_correction.toString().c_str());

	if (!((_correction.x() == _correction.x()) &&
		(_correction.y() == _correction.y()))) {
		debug(LOG_DEBUG, DEBUG_LOG, 0, "nan correction, giving up");
		return;
	}

	double	tx = 0;
	double	ty = 0;

	if (fabs(_correction.x()) > _deltat) {
		tx = (_correction.x() > 0) ? _deltat : -_deltat;
	} else {
		tx = _correction.x();
	}
	if (fabs(_correction.y()) > _deltat) {
		ty = (_correction.y() > 0) ? _deltat : -_deltat;
	} else {
		ty = _correction.y();
	}

	// make sure the time fits into the allotted time
	double	limit = 0;
	if (_sequential) {
		limit = fabs(tx) + fabs(ty);
	} else {
		limit = std::max(fabs(tx), fabs(ty));
	}
	if (limit > _deltat) {
		tx *= _deltat / limit;
		ty *= _deltat / limit;
		limit = _deltat;
	}

	// compute the activation times for the guiderport
	double	raplus = 0;
	double	raminus = 0;
	double	decplus = 0;
	double	decminus = 0;
	double	correctiontime = std::max(fabs(tx), fabs(ty));
	
	if (tx > 0) {
		raplus = tx;
	} else {
		raminus = -tx;
	}
	if (ty > 0) {
		decplus = ty;
	} else {
		decminus = -ty;
	}

	if (_sequential) {
		debug(LOG_DEBUG, DEBUG_LOG, 0, "RA movement: %.2f", tx);
		_guiderport->activate(raplus, raminus, 0, 0);
		Timer::sleep(fabs(tx));
		debug(LOG_DEBUG, DEBUG_LOG, 0, "DEC movement: %.2f", ty);
		_guiderport->activate(0, 0, decplus, decminus);
		Timer::sleep(fabs(ty));
	} else {
		// find the number of seconds, and split the correction
		// into this many single steps. This makes the 
		int	steps = 1;
		if (_stepping) {
			steps = floor(_deltat);
		}
		debug(LOG_DEBUG, DEBUG_LOG, 0, "RA/DEC %.2f/%.2f in %d steps",
			tx, ty, steps);
		raplus /= steps;
		raminus /= steps;
		decplus /= steps;
		decminus /= steps;
		int	step = 0;
		while (step++ < steps) {
			_guiderport->activate(raplus, raminus,
					decplus, decminus);
			Timer::sleep(correctiontime);
		}
	}
	
	debug(LOG_DEBUG, DEBUG_LOG, 0, "guider port action complete");
}


//////////////////////////////////////////////////////////////////////
// Specialization to GuiderPort
//////////////////////////////////////////////////////////////////////
template<>
int	ControlDevice<camera::GuiderPort,
		GuiderCalibration>::startCalibration(TrackerPtr tracker) {
	debug(LOG_DEBUG, DEBUG_LOG, 0, "GP calibration start");
	// reset the current calibration, just to make sure we don't confuse
	// it with the previous
	_calibration->reset();

	// set the focal length
	GuiderCalibration	*gcal
		= dynamic_cast<GuiderCalibration *>(_calibration);
	gcal->focallength = parameter(std::string("focallength"), 1.0);
	debug(LOG_DEBUG, DEBUG_LOG, 0, "focallength = %.3f", gcal->focallength);

	// create the calibration process
	CalibrationProcess	*calibrationprocess
		= new CalibrationProcess(_guider, _device, tracker, _database);
	process = BasicProcessPtr(calibrationprocess);

	// set the device specific
	calibrationprocess->focallength(gcal->focallength);

	// compute angular size of pixels
	gcal->masPerPixel = (_guider->pixelsize() / gcal->focallength)
				* (180 * 3600 * 1000 / M_PI);

	// start the process and update the record in the database
	return ControlDeviceBase::startCalibration(tracker);
}

/**
 * \brief apply a correction and send it to the GuiderPort
 */
template<>
Point	ControlDevice<camera::GuiderPort,
		GuiderCalibration>::correct(const Point& point, double Deltat,
			bool stepping) {
	debug(LOG_DEBUG, DEBUG_LOG, 0, "guiderport correction %s, %.2f",
		point.toString().c_str(), Deltat);
	// give up if not configured
	if (!_calibration->complete()) {
		return point;
	}

	// now compute the correction based on the calibration
	Point	correction = _calibration->correction(point, Deltat);

	// apply the correction to the guider port
	debug(LOG_DEBUG, DEBUG_LOG, 0, "apply GP correction: %s",
		correction.toString().c_str());
	double	dt = (Deltat > 0.5) ? (Deltat - 0.5) : 0;
	GuiderPortAction	*action = new GuiderPortAction(_device,
					correction, dt);
	action->stepping(stepping);
	ActionPtr	aptr(action);
	asynchronousaction.execute(aptr);

	// log the information to the callback
	TrackingPoint	ti;
	ti.t = Timer::gettime();
	ti.trackingoffset = point;
	ti.correction = correction;
	ti.type = BasicCalibration::GP;
	_guider->callback(ti);

	// no remaining error after a guider port correction ;-)
	return Point(0, 0);
}

//////////////////////////////////////////////////////////////////////
// Specialization to AdaptiveOptics
//////////////////////////////////////////////////////////////////////
template<>
int	ControlDevice<camera::AdaptiveOptics,
		AdaptiveOpticsCalibration>::startCalibration(
			TrackerPtr tracker) {
	_calibration->calibrationtype(BasicCalibration::AO);

	// create a new calibraiton process
	AOCalibrationProcess	*aocalibrationprocess
		= new AOCalibrationProcess(_guider, _device, tracker,
			_database);
	process = BasicProcessPtr(aocalibrationprocess);

	// start the calibration
	return ControlDeviceBase::startCalibration(tracker);
}

/**
 * \brief Apply correction to adaptive optics device
 */
template<>
Point	ControlDevice<camera::AdaptiveOptics,
		AdaptiveOpticsCalibration>::correct(const Point& point,
			double Deltat, bool /* stepping */) {
	// give up if not configured
	if (!_calibration->complete()) {
		debug(LOG_DEBUG, DEBUG_LOG, 0, "AO not calibrated");
		return point;
	}

	// now compute the calibration
	Point	correction = _calibration->correction(point, Deltat);

	// get the current correction
	Point	newposition = _device->get() + correction;
	try {
		_device->set(newposition);
	} catch (const std::exception& x) {
		debug(LOG_ERR, DEBUG_LOG, 0, "cannot set new position %s: %s",
			newposition.toString().c_str(), x.what());
		correction = Point(0, 0);
	} catch (...) {
		debug(LOG_ERR, DEBUG_LOG, 0, "cannot set new position %s",
			newposition.toString().c_str());
		correction = Point(0, 0);
	}

	// log the information to the callback
	TrackingPoint	ti;
	ti.t = Timer::gettime();
	ti.trackingoffset = point;
	ti.correction = correction;
	ti.type = BasicCalibration::AO;
	_guider->callback(ti);

	// get the remaining correction
	return _calibration->offset(_device->get() * -1.);
}

} // namespace guiding
} // namespace astro
