/*
 * QsiGuidePort.cpp -- QSI filter wheel implementation
 *
 * (c) 2013 Prof Dr Andreas Mueller, Hochschule Rapperswil
 */
#include <QsiGuidePort.h>
#include <AstroExceptions.h>
#include <AstroDebug.h>
#include <AstroFormat.h>

namespace astro {
namespace camera {
namespace qsi {

QsiGuidePort::QsiGuidePort(QsiCamera& camera)
	: GuidePort(DeviceName(camera.name(), DeviceName::Guideport,
		"guideport")),
	  _camera(camera) {
}

QsiGuidePort::~QsiGuidePort() {
}

uint8_t	QsiGuidePort::active() {
	std::unique_lock<std::recursive_mutex>	lock(_camera.mutex);
	bool	guiding;
	int	rc = _camera.camera().get_IsPulseGuiding(&guiding);
	if (rc == 0) {
		return (guiding) ? 0xf : 0x0;
	}
	std::string	msg = stringprintf("IsPulseGuiding: %d", rc);
	debug(LOG_ERR, DEBUG_LOG, 0, "%s", msg.c_str());
	throw std::runtime_error(msg);
}

static long	milliseconds(float time) {
	long	result = 1000 * time;
	return result;
}

void	QsiGuidePort::activate(float raplus, float raminus,
		float decplus, float decminus) {
	if ((raplus > 0) && (raminus > 0)) {
		throw std::invalid_argument("cannot activate both RA lines");
	}
	if ((decplus > 0) && (decminus > 0)) {
		throw std::invalid_argument("cannot activate both DEC lines");
	}
	std::unique_lock<std::recursive_mutex>	lock(_camera.mutex);
	if (raplus > 0) {
		debug(LOG_DEBUG, DEBUG_LOG, 0, "activate guideEast for %.3f",
			raplus);
		try {
			_camera.camera().PulseGuide(QSICamera::guideEast,
				milliseconds(raplus));
		} catch (const std::exception& x) {
			debug(LOG_ERR, DEBUG_LOG, 0, "can't guideEast/%.3f: %s",
				raplus, x.what());
			throw x;
		}
	}
	if (raminus > 0) {
		debug(LOG_DEBUG, DEBUG_LOG, 0, "activate guideWest for %.3f",
			raminus);
		try {
			_camera.camera().PulseGuide(QSICamera::guideWest,
				milliseconds(raminus));
		} catch (const std::exception& x) {
			debug(LOG_ERR, DEBUG_LOG, 0, "can't guideWest/%.3f: %s",
				raminus, x.what());
			throw x;
		}
	}
	if (decplus > 0) {
		debug(LOG_DEBUG, DEBUG_LOG, 0, "activate guideNorth for %.3f",
			decplus);
		try {
			_camera.camera().PulseGuide(QSICamera::guideNorth,
				milliseconds(decplus));
		} catch (const std::exception& x) {
			debug(LOG_ERR, DEBUG_LOG, 0,
				"can't guideNorth/%.3f: %s",
				decplus, x.what());
			throw x;
		}
	}
	if (decminus > 0) {
		debug(LOG_DEBUG, DEBUG_LOG, 0, "activate guideSouth for %.3f",
			decminus);
		try {
			_camera.camera().PulseGuide(QSICamera::guideSouth,
				milliseconds(decminus));
		} catch (const std::exception& x) {
			debug(LOG_ERR, DEBUG_LOG, 0,
				"can't guideSouth/%.3f: %s",
				decminus, x.what());
			throw x;
		}
	}
}

} // namespace qsi
} // namespace camera
} // namespace astro