/*
 * Simulator.cpp -- guiding simulator camera
 *
 * (c) 2013 Prof Dr Andreas Mueller, Hochschule Rapperswil
 */
#include <Simulator.h>
#include <AstroAdapter.h>
#include <debug.h>
#include <unistd.h>

using namespace astro::image;
using namespace astro::camera;

namespace astro {
namespace camera {
namespace sim {

static double	now() {
	struct timeval	now;
	gettimeofday(&now, NULL);
	return now.tv_sec + 0.000001 * now.tv_usec;
}

//////////////////////////////////////////////////////////////////////
// Simulator camera implementation
//////////////////////////////////////////////////////////////////////

SimCamera::SimCamera() {
	CcdInfo	ccd0;
	ccd0.size = ImageSize(640, 480);
	ccd0.name = "primary ccd";
	ccd0.binningmodes.insert(Binning(1, 1));
	ccdinfo.push_back(ccd0);
	// initial star position
	x = 320;
	y = 240;

	// movement definition
	vx = 0.1;
	vy = 0.2;
	delta = 10;
	ra.clear();
	dec.clear();
	ra.alpha = 1;
	dec.alpha = ra.alpha + M_PI / 2;

	// neither movement nor exposures are active
	exposurestart = -1;
	lastexposure = now();

	// create a mutex to protect the movement structures
	pthread_mutex_init(&mutex, NULL);
}

SimCamera::~SimCamera() {
	pthread_mutex_destroy(&mutex);
}

CcdPtr	SimCamera::getCcd(size_t id) {
	return CcdPtr(new SimCcd(ccdinfo[0], *this));
}

uint8_t	SimCamera::active() {
	return 0;
}

/**
 * \brief complete the RA movement
 */
void	SimCamera::complete(movement& mov) {
	// find out whether some movement is in progress
	if (mov.starttime > 0) {
		double	interval = mov.duration;
		double	nowtime = now();
		if (nowtime < (mov.starttime + mov.duration)) {
			interval = (mov.starttime + mov.duration - nowtime);
		}
		debug(LOG_DEBUG, DEBUG_LOG, 0, "moving for %.3f seconds",
			interval);

		// add the movement to the coordinates
		x += mov.direction * interval * delta * cos(mov.alpha);
		y += mov.direction * interval * delta * sin(mov.alpha);
		debug(LOG_DEBUG, DEBUG_LOG, 0, "new coordinates: (%f, %f)",
			x, y);

		// leave the remaining movement active
		mov.duration -= interval;
		if (mov.duration > 0) {
			mov.starttime = nowtime;
			debug(LOG_DEBUG, DEBUG_LOG, 0,
				"remaining move time: %f", mov.duration);
		} else {
			mov.starttime = -1;
		}
	} else {
		debug(LOG_DEBUG, DEBUG_LOG, 0, "no movement in progress");
	}
}

void	SimCamera::complete_movement() {
	debug(LOG_DEBUG, DEBUG_LOG, 0, "completing RA movement");
	complete(ra);
	debug(LOG_DEBUG, DEBUG_LOG, 0, "completing DEC movement");
	complete(dec);
	debug(LOG_DEBUG, DEBUG_LOG, 0, "position now: (%.3f,%.3f)", x, y);
}

/**
 * \brief Activate the Guiderport of the simulator camera
 *
 * This method completes the movement that was already in progress, as
 * far as time has already progressed.
 */
void	SimCamera::activate(float raplus, float raminus,
		float decplus, float decminus) {
	debug(LOG_DEBUG, DEBUG_LOG, 0, "moving ra+ = %.3f, ra- = %.3f, "
		"dec+ = %.3f, dec- = %.3f", raplus, raminus, decplus, decminus);

	// ensure noboy else can change the structures
	pthread_mutex_lock(&mutex);

	// complete any pending movement
	complete_movement();

	// set the new movement state
	double	movestart = now();
	debug(LOG_DEBUG, DEBUG_LOG, 0, "movement start time: %.3f", movestart);

	// right ascension
	ra.clear();
	if (raplus > 0) {
		ra.starttime = movestart;
		ra.direction = 1;
		ra.duration = raplus;
		debug(LOG_DEBUG, DEBUG_LOG, 0, "RA+ for %.3f seconds",
			ra.duration);
	} else {
		if (raminus > 0) {
			ra.starttime = movestart;
			ra.direction = -1;
			ra.duration = raminus;
			debug(LOG_DEBUG, DEBUG_LOG, 0, "RA- for %.3f seconds",
				ra.duration);
		}
	}

	// declination
	dec.clear();
	if (decplus > 0) {
		dec.starttime = movestart;
		dec.direction = 1;
		dec.duration = decplus;
		debug(LOG_DEBUG, DEBUG_LOG, 0, "DEC+ for %.3f seconds",
			dec.duration);
	} else {
		if (decminus > 0) {
			dec.starttime = movestart;
			dec.direction = -1;
			dec.duration = decminus;
			debug(LOG_DEBUG, DEBUG_LOG, 0, "DEC- for %.3f seconds",
				dec.duration);
		}
	}
	pthread_mutex_unlock(&mutex);
}

void	SimCamera::startExposure(const Exposure& _exposure) {
	exposure = _exposure;
	exposurestart = now();
}

Exposure::State	SimCamera::exposureStatus() {
	if (exposurestart < 0) {
		return Exposure::idle;
	}
	double	nowtime = now();
	if (nowtime < exposurestart + exposure.exposuretime) {
		return Exposure::exposing;
	}
	return Exposure::exposed;
}

void	SimCamera::await_exposure() {
	// compute remaining exposure time
	double	nowtime = now();
	double	exposed = nowtime - exposurestart;
	if (exposure.exposuretime > exposed) {
		double	remaining = exposure.exposuretime - exposed;
		debug(LOG_DEBUG, DEBUG_LOG, 0,
			"remaining time to exposure: %.3f", remaining);
		int	useconds = 1000000 * remaining;
		usleep(useconds);
		debug(LOG_DEBUG, DEBUG_LOG, 0, "exposure copmlete now");
	}
}

ImagePtr	SimCamera::getImage() {
	debug(LOG_DEBUG, DEBUG_LOG, 0, "retrieving image");
	// check whether the image is ready
	switch (exposureStatus()) {
	case Exposure::idle:
		throw std::runtime_error("camera idle");
		break;
	case Exposure::exposed:
		break;
	case Exposure::exposing:
		await_exposure();
		break;
	case Exposure::cancelling:
		throw std::runtime_error("cannot happen");
		break;
	}
	exposurestart = -1;

	// complete any pending motions
	debug(LOG_DEBUG, DEBUG_LOG, 0, "complete movement up to now");
	pthread_mutex_lock(&mutex);
	complete_movement();
	pthread_mutex_unlock(&mutex);

	// add base motion
	double	nowtime = now();
	x += vx * (nowtime - lastexposure);
	y += vy * (nowtime - lastexposure);
	lastexposure = nowtime;

	// create the image based on the current position parameters
	debug(LOG_DEBUG, DEBUG_LOG, 0, "creating 640x480 image");
	Image<unsigned short>	image(640, 480);
	// write image contents
	debug(LOG_DEBUG, DEBUG_LOG, 0, "drawing star at %f,%f", x, y);
	for (unsigned int xi = 0; xi < 640; xi++) {
		for (unsigned int yi = 0; yi < 480; yi++) {
			double	r = hypot(xi - x, yi - y);
			unsigned short	value = 10000 * exp(-r * r / 5);
			image.pixel(xi, yi) = value;
		}
	}
	// now extract the window defiend in the frame
	debug(LOG_DEBUG, DEBUG_LOG, 0, "extracting %s window",
		exposure.frame.toString().c_str());
	WindowAdapter<unsigned short>	wa(image, exposure.frame);
	return ImagePtr(new Image<unsigned short>(wa));
}

//////////////////////////////////////////////////////////////////////
// Simulator CCD implementation
//////////////////////////////////////////////////////////////////////
void	SimCcd::startExposure(const Exposure& exposure) throw (not_implemented) {
	camera.startExposure(exposure);
}

Exposure::State	SimCcd::exposureStatus() throw (not_implemented) {
	return camera.exposureStatus();
}

ImagePtr	SimCcd::getImage() throw (not_implemented) {
	return camera.getImage();
}

//////////////////////////////////////////////////////////////////////
// Simulator Guiderport implementation
//////////////////////////////////////////////////////////////////////

SimGuiderPort::SimGuiderPort(SimCamera& _camera) : camera(_camera) {
}

SimGuiderPort::~SimGuiderPort() {
}

uint8_t	SimGuiderPort::active() {
	return camera.active();
}

void	SimGuiderPort::activate(float raplus, float raminus, float decplus,
		float decminus) {
	camera.activate(raplus, raminus, decplus, decminus);
}

GuiderPortPtr	SimCamera::getGuiderPort() throw (not_implemented) {
	return GuiderPortPtr(new SimGuiderPort(*this));
}

} // namespace sim
} // namespace camera
} // namespace astro
