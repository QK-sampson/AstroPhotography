/*
 * monitorcmd.cpp -- monitoring related commands
 *
 * (c) 2016 Prof Dr Andreas Mueller, Hochschule Rapperswil
 */
#include "guide.h"
#include <cstdlib>
#include <iostream>
#include "monitor.h"

namespace snowstar {
namespace app {
namespace snowguide {

/**
 * \brief Implementation of the monitor command
 */
int	Guide::monitor_command(GuiderPrx guider) {
	// The type of callback to install depends on the current guider state
	switch (guider->getState()) {
	case GuiderUNCONFIGURED:
	case GuiderIDLE:
	case GuiderCALIBRATED:
	case GuiderIMAGING:
	case GuiderDARKACQUIRE:
	case GuiderFLATACQUIRE:
		std::cout << "not in monitorable state" << std::endl;
	case GuiderCALIBRATING:
		return monitor_calibration(guider);
	case GuiderGUIDING:
		return monitor_guiding(guider);
	}
	return EXIT_FAILURE;
}

} // namespace snowguide
} // namespace app
} // namespace snowstar
