/*
 * SxGuiderPort.cpp -- guider port implementation
 *
 * (c) 2013 Prof Dr Andreas Mueller, Hochschule Rapperswil
 */
#include <SxGuiderPort.h>
#include <AstroDebug.h>
#include <algorithm>
#include <SxUtils.h>

namespace astro {
namespace camera {
namespace sx {

#define	SX_RAPLUS_BIT	1
#define SX_DECPLUS_BIT	2
#define SX_DECMINUS_BIT	4
#define	SX_RAMINUS_BIT	8

SxGuiderPort::SxGuiderPort(SxCamera& _camera)
	: BasicGuiderport(GuiderPort::defaultname(_camera.name(), "guiderport")),
	  camera(_camera) {
}

SxGuiderPort::~SxGuiderPort() {
}

void	SxGuiderPort::do_activate(uint8_t active) {
	uint8_t	newstate = 0;
	newstate |= (active & RAPLUS) ? SX_RAPLUS_BIT : 0;
	newstate |= (active & RAMINUS) ? SX_RAMINUS_BIT : 0;
	newstate |= (active & DECPLUS) ? SX_DECPLUS_BIT : 0;
	newstate |= (active & DECMINUS) ? SX_DECMINUS_BIT : 0;
	debug(LOG_DEBUG, DEBUG_LOG, 0, "new port state: %02x", newstate);

	// now set the new state
	EmptyRequest	request(
		RequestBase::vendor_specific_type,
		RequestBase::device_recipient, 0,
		(uint8_t)SX_CMD_SET_STAR2K, (uint16_t)newstate);
	camera.controlRequest(&request);
}

} // namespace sx
} // namespace camera
} // namespace astro
