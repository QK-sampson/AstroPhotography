/*
 * SbigCooler.cpp -- SBIG cooler implementation
 *
 * (c) 2013 Prof Dr Andreas Mueller, Hochschule Rapperswil
 */
#ifdef HAVE_CONFIG_H
#include <config.h>
#endif /* HAVE_CONFIG_H */

#ifdef HAVE_LPARDRV_H
#include <lpardrv.h>
#else
#ifdef HAVE_SBIGUDRV_LPARDRV_H
#include <SBIGUDrv/lpardrv.h>
#endif /* HAVE_SBIGUDRV_LPARDRV_H */
#endif

#include <SbigLocator.h>
#include <SbigCooler.h>
#include <utils.h>
#include <AstroDebug.h>

namespace astro {
namespace camera {
namespace sbig {

/**
 * \brief Create an SBIG cooler
 *
 * This is essentiall a holder for cooler specific state and a reference
 * to the camera. The camera contains all the information needed to perform
 * a call to the SBIG universal driver library.
 */
SbigCooler::SbigCooler(SbigCamera& _camera) : camera(_camera) {
	debug(LOG_DEBUG, DEBUG_LOG, 0, "constructing cooler");
	enabled = isOn();
	getSetTemperature();
}

SbigCooler::~SbigCooler() {
}

/**
 * \brief Query the set temperature
 */
float	SbigCooler::getSetTemperature() {
	SbigLock	lock;
	camera.sethandle();
	debug(LOG_DEBUG, DEBUG_LOG, 0, "retrieve set temperature");
	QueryTemperatureStatusParams	params;
	QueryTemperatureStatusResults2	results;
	params.request = TEMP_STATUS_ADVANCED2;
	short	e = SBIGUnivDrvCommand(CC_QUERY_TEMPERATURE_STATUS,
		&params, &results);
	if (e != CE_NO_ERROR) {
		debug(LOG_ERR, DEBUG_LOG, 0, "cannot get set temperature : %s",
			sbig_error(e).c_str());
		throw SbigError(e);
	}
	return temperature = results.ccdSetpoint + 273.1;
}

/**
 * \brief Query the actual temperature
 */
float	SbigCooler::getActualTemperature() {
	SbigLock	lock;
	camera.sethandle();
	debug(LOG_DEBUG, DEBUG_LOG, 0, "get actual temperature");
	QueryTemperatureStatusParams	params;
	QueryTemperatureStatusResults2	results;
	params.request = TEMP_STATUS_ADVANCED2;
	short	e = SBIGUnivDrvCommand(CC_QUERY_TEMPERATURE_STATUS,
		&params, &results);
	if (e != CE_NO_ERROR) {
		debug(LOG_ERR, DEBUG_LOG, 0, "cannot get temperature : %s",
			sbig_error(e).c_str());
		throw SbigError(e);
	}
	return results.imagingCCDTemperature + 273.1;
}

/**
 * \brief Set the set temperature
 */
void	SbigCooler::setTemperature(const float temperature) {
	SbigLock	lock;
	camera.sethandle();
	debug(LOG_DEBUG, DEBUG_LOG, 0, "set the set temperature");
	this->temperature = temperature;
	if (!isOn()) {
		return;
	}
	set();
}

/**
 * \brief Common (private) set function
 */
void	SbigCooler::set() {
	SbigLock	lock;
	camera.sethandle();
	debug(LOG_DEBUG, DEBUG_LOG, 0, "set parameters");
	SetTemperatureRegulationParams2	params;
	params.regulation = (enabled) ? REGULATION_ON : REGULATION_OFF;
	params.ccdSetpoint = temperature - 273.1;
	short	e = SBIGUnivDrvCommand(CC_SET_TEMPERATURE_REGULATION2,
		&params, NULL);
	if (e != CE_NO_ERROR) {
		debug(LOG_ERR, DEBUG_LOG, 0, "cannot control temperature "
			"regulation: %s", sbig_error(e).c_str());
		throw SbigError(e);
	}
}

/**
 * \brief Query whether cooler is on
 */
bool	SbigCooler::isOn() {
	SbigLock	lock;
	camera.sethandle();
	debug(LOG_DEBUG, DEBUG_LOG, 0, "query regulation status");
	QueryTemperatureStatusParams	params;
	QueryTemperatureStatusResults	results;
	params.request = TEMP_STATUS_STANDARD;
	short	e = SBIGUnivDrvCommand(CC_QUERY_TEMPERATURE_STATUS,
		&params, &results);
	if (e != CE_NO_ERROR) {
		debug(LOG_ERR, DEBUG_LOG, 0, "cannot get temperature "
			"status: %s", sbig_error(e).c_str());
		throw SbigError(e);
	}
	return (results.enabled) ? true : false;
}

/**
 * \brief Turn cooler on.
 */
void	SbigCooler::setOn(bool onoff) {
	enabled = onoff;
	set();
}

} // namespace sbig
} // namespace camera
} // namespace astro
