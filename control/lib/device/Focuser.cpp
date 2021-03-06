/*
 * Focuser.cpp -- Focuser base class
 *
 * (c) 2013 Prof Dr Andreas Mueller, Hochschule Rapperswil
 */
#include <AstroCamera.h>
#include <AstroExceptions.h>
#include <AstroDebug.h>
#include <includes.h>

using namespace astro::device;

namespace astro {
namespace camera {

DeviceName::device_type	Focuser::devicetype = DeviceName::Focuser;

Focuser::Focuser(const DeviceName& name) : Device(name, DeviceName::Focuser) {
}

Focuser::Focuser(const std::string& name) : Device(name, DeviceName::Focuser) {
}

Focuser::~Focuser() {
}

long	Focuser::min() {
	return 0;
}

long	Focuser::max() {
	return std::numeric_limits<unsigned short>::max();
}

long	Focuser::current() {
	throw NotImplemented("base Focuser does not implement current method");
}

long	Focuser::backlash() {
	if (hasProperty("backlash")) {
		return std::stoi(getProperty("backlash"));
	}
	return 0;
}

void	Focuser::set(long /* value */) {
	throw NotImplemented("base Focuser does not implement set method");
}

/**
 * \brief Position focuser and wait for completion
 *
 * This method calls the set method but then waits until either
 * the position is reached, or the timeout occurs.
 * \param value		new focuser position
 * \param timeout	maximum numer of seconds to wait for the focuser to
 *			reach the new position
 */
bool	Focuser::moveto(long value, unsigned long timeout) {
	debug(LOG_DEBUG, DEBUG_LOG, 0, "moving to %hu", value);
	// record current time
	time_t	starttime;
	time(&starttime);
	time_t	timelimit = starttime + timeout;

	// ensure we stay within the limits
	if (value > max()) {
		value = max();
	}
	if (value < min()) {
		value = min();
	}

	// start moving to this position
	set(value);

	// wait until we reach the position
	time_t	now;
	time(&now);
	long	currentposition = current();
	do {
		if (value != currentposition) {
			usleep(100000);
		}
		currentposition = current();
		time(&now);
	} while ((currentposition != value) && (now < timelimit));
	debug(LOG_DEBUG, DEBUG_LOG, 0, "final position is %hu after %d seconds",
		currentposition, now - starttime);

	// report whether we have reached the position
	return (currentposition == value);
}

} // namespace camera
} // namespace astro
