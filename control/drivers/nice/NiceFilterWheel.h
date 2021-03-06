/*
 * NiceFilterWheel.h -- wrapper for filter wheels
 *
 * (c) 2015 Prof Dr Andreas Mueller, Hochschule Rapperswil 
 */
#ifndef _NiceFilterWheel_h
#define _NiceFilterWheel_h

#include <AstroCamera.h>
#include <NiceDevice.h>
#include <camera.h>

namespace astro {
namespace camera {
namespace nice {

class NiceFilterWheel : public FilterWheel, public NiceDevice {
	snowstar::FilterWheelPrx	_filterwheel;
public:
	NiceFilterWheel(snowstar::FilterWheelPrx filterwheel,
		const DeviceName& devicename);
	virtual ~NiceFilterWheel();

	virtual unsigned int	nFilters();
	virtual unsigned int	currentPosition();
	virtual void	select(size_t filterindex);
	virtual void	select(const std::string& name);
	virtual std::string	filterName(size_t filterindex);
	virtual State	getState();
};

} // namespace nice
} // namespace camera
} // namespace astro

#endif /* _NiceFilterWheel_h */
